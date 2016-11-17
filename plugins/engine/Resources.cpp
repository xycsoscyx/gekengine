#include "GEK\Utility\String.hpp"
#include "GEK\Utility\ThreadPool.hpp"
#include "GEK\Utility\ShuntingYard.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\JSON.hpp"
#include "GEK\Shapes\Sphere.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Visual.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\Engine\Shader.hpp"
#include "GEK\Engine\Filter.hpp"
#include "GEK\Engine\Material.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\Component.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Components\Light.hpp"
#include "GEK\Components\Color.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        static ShuntingYard shuntingYard;

        GEK_INTERFACE(ResourceRequester)
        {
            virtual void addRequest(std::function<void(void)> &&load) = 0;
        };

        template <class HANDLE, typename TYPE>
        class ResourceCache
        {
        public:
            using TypePtr = std::shared_ptr<TYPE>;
            using ResourceHandleMap = concurrency::concurrent_unordered_map<std::size_t, HANDLE>;
            using ResourceMap = concurrency::concurrent_unordered_map<HANDLE, TypePtr>;

        private:
            uint32_t nextIdentifier = 0;

        protected:
            ResourceRequester *resources = nullptr;
            ResourceHandleMap resourceHandleMap;
            ResourceMap resourceMap;

        public:
            ResourceCache(ResourceRequester *resources)
                : resources(resources)
            {
                GEK_REQUIRE(resources);
            }

            virtual ~ResourceCache(void) = default;

            virtual void clear(void)
            {
                resourceHandleMap.clear();
                resourceMap.clear();
            }

            void setResource(HANDLE handle, const TypePtr &data)
            {
                resourceMap[handle] = data;
            }

            virtual TYPE * const getResource(HANDLE handle) const
            {
                auto resourceSearch = resourceMap.find(handle);
                if (resourceSearch != std::end(resourceMap))
                {
                    return resourceSearch->second.get();
                }

                return nullptr;
            }

            uint32_t getNextHandle(void)
            {
                return InterlockedIncrement(&nextIdentifier);
            }
        };

        template <class HANDLE, typename TYPE>
        class GeneralResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        private:
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;

        public:
            GeneralResourceCache(ResourceRequester *resources)
                : ResourceCache(resources)
            {
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                ResourceCache::clear();
            }

            HANDLE getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        handle = resourceSearch->second;
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    resources->addRequest([this, handle, load = move(load)](void) -> void
                    {
                        setResource(handle, load(handle));
                    });
                }

                return handle;
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <class HANDLE, typename TYPE>
        class DynamicResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        private:
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;
            concurrency::concurrent_unordered_map<HANDLE, std::size_t> loadParameters;

        public:
            DynamicResourceCache(ResourceRequester *resources)
                : ResourceCache(resources)
            {
            }

            void clear(void)
            {
                loadParameters.clear();
                requestedLoadSet.clear();
                ResourceCache::clear();
            }

            HANDLE getHandle(std::size_t hash, std::size_t parameters, std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        handle = resourceSearch->second;
                        auto loadParametersSearch = loadParameters.find(handle);
                        if (loadParametersSearch == std::end(loadParameters) || loadParametersSearch->second != parameters)
                        {
                            loadParameters[handle] = parameters;
                            resources->addRequest([this, handle, load = move(load)](void) -> void
                            {
                                setResource(handle, load(handle));
                            });
                        }
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    loadParameters[handle] = parameters;
                    resources->addRequest([this, handle, load = move(load)](void) -> void
                    {
                        setResource(handle, load(handle));
                    });
                }

                return handle;
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <class HANDLE, typename TYPE>
        class ProgramResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        public:
            ProgramResourceCache(ResourceRequester *resources)
                : ResourceCache(resources)
            {
            }

            HANDLE getHandle(std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                handle = getNextHandle();
                resources->addRequest([this, handle, load = move(load)](void) -> void
                {
                    setResource(handle, load(handle));
                });

                return handle;
            }
        };

        template <class HANDLE, typename TYPE>
        class ReloadResourceCache
            : public ResourceCache<HANDLE, TYPE>
        {
        private:
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;

        public:
            ReloadResourceCache(ResourceRequester *resources)
                : ResourceCache(resources)
            {
            }

            void reload(void)
            {
                for (auto &resourceSearch : resourceMap)
                {
                    auto resource = resourceSearch.second.get();
                    if (resource)
                    {
                        resource->reload();
                    }
                }
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                ResourceCache::clear();
            }

            HANDLE getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        handle = resourceSearch->second;
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    auto &resource = resourceMap[handle];
                    setResource(handle, load(handle));
                }

                return handle;
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <typename TYPE>
        struct ObjectCache
        {
            std::vector<TYPE *> objectList;

            template <typename INPUT, typename HANDLE, typename SOURCE>
            bool set(const std::vector<INPUT> &inputList, ResourceCache<HANDLE, SOURCE> &cache)
            {
                auto listCount = inputList.size();
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    auto resource = cache.getResource(inputList[object]);
                    if (!resource)
                    {
                        return false;
                    }

                    objectList[object] = dynamic_cast<TYPE *>(resource);
                    if (!objectList[object])
                    {
                        return false;
                    }
                }

                return true;
            }

            template <typename INPUT, typename HANDLE>
            bool set(const std::vector<INPUT> &inputList, ResourceCache<HANDLE, TYPE> &cache)
            {
                auto listCount = inputList.size();
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    auto resource = cache.getResource(inputList[object]);
                    if (!resource)
                    {
                        return false;
                    }

                    objectList[object] = resource;
                }

                return true;
            }

            std::vector<TYPE *> &get(void)
            {
                return objectList;
            }

            const std::vector<TYPE *> &get(void) const
            {
                return objectList;
            }
        };

        GEK_CONTEXT_USER(Resources, Plugin::Core *, Video::Device *)
            , public Engine::Resources
            , public ResourceRequester
        {
        private:
            Plugin::Core *core = nullptr;
            Video::Device *videoDevice = nullptr;

            ThreadPool loadPool;
            std::recursive_mutex shaderMutex;

            ProgramResourceCache<ProgramHandle, Video::Object> programCache;
            GeneralResourceCache<VisualHandle, Plugin::Visual> visualCache;
            GeneralResourceCache<MaterialHandle, Engine::Material> materialCache;
            ReloadResourceCache<ShaderHandle, Engine::Shader> shaderCache;
            ReloadResourceCache<ResourceHandle, Engine::Filter> filterCache;
            DynamicResourceCache<ResourceHandle, Video::Object> dynamicCache;
            GeneralResourceCache<RenderStateHandle, Video::Object> renderStateCache;
            GeneralResourceCache<DepthStateHandle, Video::Object> depthStateCache;
            GeneralResourceCache<BlendStateHandle, Video::Object> blendStateCache;

            concurrency::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

            struct Validate
            {
                bool state;
                Validate(void)
                    : state(false)
                {
                }

                operator bool ()
                {
                    return state;
                }

                bool operator = (bool state)
                {
                    this->state = state;
                    return state;
                }
            };

            Validate drawPrimitiveValid;
            Validate dispatchValid;

            Video::TargetPtr screen;
            Video::TargetPtr screenBuffer;

        public:
            Resources(Context *context, Plugin::Core *core, Video::Device *videoDevice)
                : ContextRegistration(context)
                , core(core)
                , videoDevice(videoDevice)
                , programCache(this)
                , visualCache(this)
                , materialCache(this)
                , shaderCache(this)
                , filterCache(this)
                , dynamicCache(this)
                , renderStateCache(this)
                , depthStateCache(this)
                , blendStateCache(this)
                , loadPool(2)
            {
                GEK_REQUIRE(core);
                GEK_REQUIRE(videoDevice);

                core->onResize.connect<Resources, &Resources::onResize>(this);
            }

            ~Resources(void)
            {
                GEK_REQUIRE(core);

                core->onResize.disconnect<Resources, &Resources::onResize>(this);
            }

            Validate &getValid(Video::Device::Context::Pipeline *videoPipeline)
            {
                GEK_REQUIRE(videoPipeline);

                return (videoPipeline->getType() == Video::PipelineType::Compute ? dispatchValid : drawPrimitiveValid);
            }

            Video::TexturePtr loadTextureData(const wchar_t *textureName, uint32_t flags)
            {
                GEK_REQUIRE(textureName);

                // iterate over formats in case the texture name has no extension
                static const String formatList[] =
                {
                    L"",
                    L".dds",
                    L".tga",
                    L".png",
                    L".jpg",
                    L".bmp",
                };

                String baseFileName(getContext()->getFileName(L"data\\textures", textureName));
                for (auto &format : formatList)
                {
                    String fileName(baseFileName);
                    fileName.append(format);

                    if (FileSystem::IsFile(fileName))
                    {
                        auto texture = videoDevice->loadTexture(fileName, flags);
                        texture->setName(fileName);
                        return texture;
                    }
                }

                return nullptr;
            }

            Video::TexturePtr createTextureData(const String &pattern, const String &parameters)
            {
                Video::TexturePtr texture;
                if (pattern.compareNoCase(L"color") == 0)
                {
                    uint8_t colorData[4];
                    Video::Format format = Video::Format::Unknown;
                    try
                    {
                        auto rpnTokenList = shuntingYard.getTokenList(parameters);
                        uint32_t colorSize = shuntingYard.getReturnSize(rpnTokenList);
                        if (colorSize == 1)
                        {
                            float color;
                            shuntingYard.evaluate(rpnTokenList, color);
                            colorData[0] = uint8_t(color * 255.0f);
                            format = Video::Format::R8_UNORM;
                        }
                        else if (colorSize == 2)
                        {
                            Math::Float2 color;
                            shuntingYard.evaluate(rpnTokenList, color);
                            colorData[0] = uint8_t(color.x * 255.0f);
                            colorData[1] = uint8_t(color.y * 255.0f);
                            colorData[2] = 0;
                            colorData[3] = 0;
                            format = Video::Format::R8G8_UNORM;
                        }
                        else if (colorSize == 3)
                        {
                            Math::Float3 color;
                            shuntingYard.evaluate(rpnTokenList, color);
                            colorData[0] = uint8_t(color.x * 255.0f);
                            colorData[1] = uint8_t(color.y * 255.0f);
                            colorData[2] = uint8_t(color.z * 255.0f);
                            colorData[3] = 0;
                            format = Video::Format::R8G8B8A8_UNORM;
                        }
                        else if (colorSize == 4)
                        {
                            Math::Float4 color;
                            shuntingYard.evaluate(rpnTokenList, color);
                            colorData[0] = uint8_t(color.x * 255.0f);
                            colorData[1] = uint8_t(color.y * 255.0f);
                            colorData[2] = uint8_t(color.z * 255.0f);
                            colorData[3] = uint8_t(color.w * 255.0f);
                            format = Video::Format::R8G8B8A8_UNORM;
                        }
                    }
                    catch (const ShuntingYard::Exception &)
                    {
                    };

                    if (format == Video::Format::Unknown)
                    {
                        throw InvalidParameter("Invalid color format encountered");
                    }

                    texture = videoDevice->createTexture(format, 1, 1, 1, 1, Video::TextureFlags::Resource, colorData);
                }
                else if (pattern.compareNoCase(L"normal") == 0)
                {
                    Math::Float3 normal;
                    try
                    {
                        shuntingYard.evaluate(parameters, normal);
                    }
                    catch (const ShuntingYard::Exception &)
                    {
                        normal.set(0.0f, 0.0f, 0.0f);
                    }

                    uint8_t normalData[4] =
                    {
                        uint8_t(((normal.x + 1.0f) * 0.5f) * 255.0f),
                        uint8_t(((normal.y + 1.0f) * 0.5f) * 255.0f),
                        uint8_t(((normal.z + 1.0f) * 0.5f) * 255.0f),
                        255,
                    };

                    texture = videoDevice->createTexture(Video::Format::R8G8B8A8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, normalData);
                }
                else if (pattern.compareNoCase(L"system") == 0)
                {
                    if (parameters.compareNoCase(L"debug") == 0)
                    {
                        uint8_t data[] =
                        {
                            255, 0, 255, 255,
                        };

                        texture = videoDevice->createTexture(Video::Format::R8G8B8A8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, data);
                    }
                    else if (parameters.compareNoCase(L"flat") == 0)
                    {
                        Math::Float3 normal(0.0f, 0.0f, 1.0f);
                        uint8_t normalData[4] =
                        {
                            uint8_t(((normal.x + 1.0f) * 0.5f) * 255.0f),
                            uint8_t(((normal.y + 1.0f) * 0.5f) * 255.0f),
                            uint8_t(((normal.z + 1.0f) * 0.5f) * 255.0f),
                            255,
                        };

                        texture = videoDevice->createTexture(Video::Format::R8G8B8A8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, normalData);
                    }
                    else
                    {
                        throw InvalidParameter("Unknown system pattern encountered");
                    }
                }
                else
                {
                    throw InvalidParameter("Unknown texture pattern encountered");
                }

                texture->setName(String::create(L"%v:%v", pattern, parameters));
                return texture;
            }

            String getFullProgram(const wchar_t *name, const wchar_t *engineData)
            {
                GEK_REQUIRE(name);
                GEK_REQUIRE(engineData);

                String rootProgramsDirectory(getContext()->getFileName(L"data\\programs"));
                String fileName(FileSystem::GetFileName(rootProgramsDirectory, name));
                if (FileSystem::IsFile(fileName))
                {
                    String programDirectory(FileSystem::GetDirectory(fileName));

                    String baseProgram;
                    FileSystem::Load(fileName, baseProgram);
                    baseProgram.replace(L"\r\n", L"\r");
                    baseProgram.replace(L"\n\r", L"\r");
                    baseProgram.replace(L"\n", L"\r");
                    auto programLines = baseProgram.split(L'\r', false);

                    String uncompiledProgram;
                    for (auto &line : programLines)
                    {
                        if (line.empty())
                        {
                            continue;
                        }
                        else if (line.find(L"#include") == 0)
                        {
                            String includeName(line.subString(8));
                            includeName.trim();

                            if (includeName.empty())
                            {
                                throw InvalidIncludeName("Empty include encountered");
                            }
                            else
                            {
                                String includeData;
                                if (includeName.compareNoCase(L"GEKEngine") == 0)
                                {
                                    includeData = engineData;
                                }
                                else
                                {
                                    auto includeType = includeName.at(0);
                                    includeName = includeName.subString(1, includeName.length() - 2);
                                    if (includeType == L'\"')
                                    {
                                        String localFileName(FileSystem::GetFileName(programDirectory, includeName));
                                        if (FileSystem::IsFile(localFileName))
                                        {
                                            FileSystem::Load(localFileName, includeData);
                                        }
                                    }
                                    else if (includeType == L'<')
                                    {
                                        String rootFileName(FileSystem::GetFileName(rootProgramsDirectory, includeName));
                                        if (FileSystem::IsFile(rootFileName))
                                        {
                                            FileSystem::Load(rootFileName, includeData);
                                        }
                                    }
                                    else
                                    {
                                        throw InvalidIncludeType("Invalid include definition encountered");
                                    }
                                }

                                uncompiledProgram.append(includeData);
                                uncompiledProgram.append(L"\r\n");
                            }
                        }
                        else
                        {
                            uncompiledProgram.append(line);
                            uncompiledProgram.append(L"\r\n");
                        }
                    }

                    return uncompiledProgram;
                }
                else
                {
                    return engineData;
                }
            }

            // Plugin::Core Slots
            void onResize(void)
            {
                programCache.clear();
                shaderCache.reload();
                filterCache.reload();

                auto backBuffer = videoDevice->getBackBuffer();
                uint32_t width = backBuffer->getWidth();
                uint32_t height = backBuffer->getHeight();
                createTexture(L"screen", Video::Format::R11G11B10_FLOAT, width, height, 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
                createTexture(L"screenBuffer", Video::Format::R11G11B10_FLOAT, width, height, 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
            }

            // ResourceRequester
            void addRequest(std::function<void(void)> &&load)
            {
                loadPool.enqueue(load);
            }

            // Plugin::Resources
            VisualHandle loadVisual(const wchar_t *visualName)
            {
                GEK_REQUIRE(visualName);

                auto load = [this, visualName = String(visualName)](VisualHandle)->Plugin::VisualPtr
                {
                    return getContext()->createClass<Plugin::Visual>(L"Engine::Visual", videoDevice, (Engine::Resources *)this, visualName);
                };

                auto hash = GetHash(visualName);
                return visualCache.getHandle(hash, std::move(load));
            }

            MaterialHandle loadMaterial(const wchar_t *materialName)
            {
                GEK_REQUIRE(materialName);

                auto load = [this, materialName = String(materialName)](MaterialHandle handle)->Engine::MaterialPtr
                {
                    return getContext()->createClass<Engine::Material>(L"Engine::Material", (Engine::Resources *)this, materialName, handle);
                };

                auto hash = GetHash(materialName);
                return materialCache.getHandle(hash, std::move(load));
            }

            ResourceHandle loadTexture(const wchar_t *textureName, uint32_t flags)
            {
                GEK_REQUIRE(textureName);

                auto load = [this, textureName = String(textureName), flags](ResourceHandle)->Video::TexturePtr
                {
                    return loadTextureData(textureName, flags);
                };

                auto hash = GetHash(textureName);
                return dynamicCache.getHandle(hash, flags, std::move(load));
            }

            ResourceHandle createTexture(const wchar_t *pattern, const wchar_t *parameters)
            {
                GEK_REQUIRE(pattern);
                GEK_REQUIRE(parameters);

                auto load = [this, pattern = String(pattern), parameters = String(parameters)](ResourceHandle)->Video::TexturePtr
                {
                    return createTextureData(pattern, parameters);
                };

                auto hash = GetHash(pattern, parameters);
                return dynamicCache.getHandle(hash, 0, std::move(load));
            }

            ResourceHandle createTexture(const wchar_t *textureName, Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t flags)
            {
                GEK_REQUIRE(textureName);

                auto load = [this, textureName = String(textureName), format, width, height, depth, mipmaps, flags](ResourceHandle)->Video::TexturePtr
                {
                    auto texture = videoDevice->createTexture(format, width, height, depth, mipmaps, flags);
                    texture->setName(textureName);
                    return texture;
                };

                auto hash = GetHash(textureName);
                auto parameters = GetHash(format, width, height, depth, mipmaps, flags);
                return dynamicCache.getHandle(hash, parameters, std::move(load));
            }

            ResourceHandle createBuffer(const wchar_t *bufferName, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const std::vector<uint8_t> &staticData)
            {
                GEK_REQUIRE(bufferName);
                
                auto load = [this, bufferName = String(bufferName), stride, count, type, flags, staticData](ResourceHandle)->Video::BufferPtr
                {
                    auto buffer = videoDevice->createBuffer(stride, count, type, flags, (void *)staticData.data());
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto hash = GetHash(bufferName);
                if (staticData.empty())
                {
                    auto parameters = GetHash(stride, count, type, flags);
                    return dynamicCache.getHandle(hash, parameters, std::move(load));
                }
                else
                {
                    return dynamicCache.getHandle(hash, reinterpret_cast<std::size_t>(staticData.data()), std::move(load));
                }
            }

            ResourceHandle createBuffer(const wchar_t *bufferName, Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const std::vector<uint8_t> &staticData)
            {
                GEK_REQUIRE(bufferName);

                auto load = [this, bufferName = String(bufferName), format, count, type, flags, staticData](ResourceHandle)->Video::BufferPtr
                {
                    auto buffer = videoDevice->createBuffer(format, count, type, flags, (void *)staticData.data());
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto hash = GetHash(bufferName);
                if (staticData.empty())
                {
                    auto parameters = GetHash(format, count, type, flags);
                    return dynamicCache.getHandle(hash, parameters, std::move(load));
                }
                else
                {
                    return dynamicCache.getHandle(hash, reinterpret_cast<std::size_t>(staticData.data()), std::move(load));
                }
            }

            void setIndexBuffer(Video::Device::Context *videoContext, ResourceHandle resourceHandle, uint32_t offset)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    auto resource = dynamicCache.getResource(resourceHandle);
                    if (drawPrimitiveValid = (resource != nullptr))
                    {
                        videoContext->setIndexBuffer(dynamic_cast<Video::Buffer *>(resource), offset);
                    }
                }
            }

            ObjectCache<Video::Buffer> vertexBufferCache;
            void setVertexBufferList(Video::Device::Context *videoContext, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstSlot, uint32_t *offsetList)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid && (drawPrimitiveValid = vertexBufferCache.set(resourceHandleList, dynamicCache)))
                {
                    videoContext->setVertexBufferList(vertexBufferCache.get(), firstSlot, offsetList);
                }
            }

            ObjectCache<Video::Buffer> constantBufferCache;
            void setConstantBufferList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage)
            {
                GEK_REQUIRE(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid && (valid = constantBufferCache.set(resourceHandleList, dynamicCache)))
                {
                    videoPipeline->setConstantBufferList(constantBufferCache.get(), firstStage);
                }
            }

            ObjectCache<Video::Object> resourceCache;
            void setResourceList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage)
            {
                GEK_REQUIRE(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid && (valid = resourceCache.set(resourceHandleList, dynamicCache)))
                {
                    videoPipeline->setResourceList(resourceCache.get(), firstStage);
                }
            }

            ObjectCache<Video::Object> unorderedAccessCache;
            void setUnorderedAccessList(Video::Device::Context::Pipeline *videoPipeline, const std::vector<ResourceHandle> &resourceHandleList, uint32_t firstStage)
            {
                GEK_REQUIRE(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid && (valid = unorderedAccessCache.set(resourceHandleList, dynamicCache)))
                {
                    videoPipeline->setUnorderedAccessList(unorderedAccessCache.get(), firstStage);
                }
            }

            void clearIndexBuffer(Video::Device::Context *videoContext)
            {
                GEK_REQUIRE(videoContext);

                videoContext->clearIndexBuffer();
            }

            void clearVertexBufferList(Video::Device::Context *videoContext, uint32_t count, uint32_t firstSlot)
            {
                GEK_REQUIRE(videoContext);

                videoContext->clearVertexBufferList(count, firstSlot);
            }

            void clearConstantBufferList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage)
            {
                GEK_REQUIRE(videoPipeline);

                videoPipeline->clearConstantBufferList(count, firstStage);
            }

            void clearResourceList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage)
            {
                GEK_REQUIRE(videoPipeline);

                videoPipeline->clearResourceList(count, firstStage);
            }

            void clearUnorderedAccessList(Video::Device::Context::Pipeline *videoPipeline, uint32_t count, uint32_t firstStage)
            {
                GEK_REQUIRE(videoPipeline);

                videoPipeline->clearUnorderedAccessList(count, firstStage);
            }

            void drawPrimitive(Video::Device::Context *videoContext, uint32_t vertexCount, uint32_t firstVertex)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawPrimitive(vertexCount, firstVertex);
                }
            }

            void drawInstancedPrimitive(Video::Device::Context *videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
            {
                if (drawPrimitiveValid)
                {
                    videoContext->drawInstancedPrimitive(instanceCount, firstInstance, vertexCount, firstVertex);
                }
            }

            void drawIndexedPrimitive(Video::Device::Context *videoContext, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
            {
                GEK_REQUIRE(videoContext);

                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawIndexedPrimitive(indexCount, firstIndex, firstVertex);
                }
            }

            void drawInstancedIndexedPrimitive(Video::Device::Context *videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    videoContext->drawInstancedIndexedPrimitive(instanceCount, firstInstance, indexCount, firstIndex, firstVertex);
                }
            }

            void dispatch(Video::Device::Context *videoContext, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
            {
                GEK_REQUIRE(videoContext);

                if (dispatchValid)
                {
                    videoContext->dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                }
            }

            // Engine::Resources
            void clear(void)
            {
                loadPool.clear();
                materialShaderMap.clear();
                programCache.clear();
                materialCache.clear();
                shaderCache.clear();
                filterCache.clear();
                dynamicCache.clear();
                renderStateCache.clear();
                depthStateCache.clear();
                blendStateCache.clear();

                auto backBuffer = videoDevice->getBackBuffer();
                uint32_t width = backBuffer->getWidth();
                uint32_t height = backBuffer->getHeight();
                createTexture(L"screen", Video::Format::R11G11B10_FLOAT, width, height, 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
                createTexture(L"screenBuffer", Video::Format::R11G11B10_FLOAT, width, height, 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
            }

            ShaderHandle getMaterialShader(MaterialHandle material) const
            {
                auto shaderSearch = materialShaderMap.find(material);
                if (shaderSearch != std::end(materialShaderMap))
                {
                    return shaderSearch->second;
                }

                return ShaderHandle();
            }

            ResourceHandle getResourceHandle(const wchar_t *resourceName) const
            {
                GEK_REQUIRE(resourceName);

                return dynamicCache.getHandle(GetHash(resourceName));
            }

            Engine::Shader * const getShader(ShaderHandle handle) const
            {
                return shaderCache.getResource(handle);
            }

            Engine::Shader * const getShader(const wchar_t *shaderName, MaterialHandle material)
            {
                GEK_REQUIRE(shaderName);

                std::unique_lock<std::recursive_mutex> lock(shaderMutex);
                auto load = [this, shaderName = String(shaderName)](ShaderHandle) mutable -> Engine::ShaderPtr
                {
                    return getContext()->createClass<Engine::Shader>(L"Engine::Shader", videoDevice, (Engine::Resources *)this, core->getPopulation(), shaderName);
                };

                auto hash = GetHash(shaderName);
                ShaderHandle shader = shaderCache.getHandle(hash, std::move(load));
                if (material)
                {
                    materialShaderMap[material] = shader;
                }

                return shaderCache.getResource(shader);
            }

            Engine::Filter * const getFilter(const wchar_t *filterName)
            {
                GEK_REQUIRE(filterName);

                auto load = [this, filterName = String(filterName)](ResourceHandle)->Engine::FilterPtr
                {
                    return getContext()->createClass<Engine::Filter>(L"Engine::Filter", videoDevice, (Engine::Resources *)this, filterName);
                };

                auto hash = GetHash(filterName);
                ResourceHandle filter = filterCache.getHandle(hash, std::move(load));
                return filterCache.getResource(filter);
            }

            std::vector<uint8_t> compileProgram(Video::PipelineType pipelineType, const wchar_t *name, const wchar_t *entryFunction, const wchar_t *engineData)
            {
                GEK_REQUIRE(name);
                GEK_REQUIRE(entryFunction);
                GEK_REQUIRE(engineData);

                auto uncompiledProgram = getFullProgram(name, engineData);

                auto hash = GetHash(uncompiledProgram);
                auto cache = String::create(L".%v.bin", hash);
                String cacheFileName(FileSystem::ReplaceExtension(getContext()->getFileName(L"data\\cache", name), cache));

				std::vector<uint8_t> compiledProgram;
                if (FileSystem::IsFile(cacheFileName))
                {
                    FileSystem::Load(cacheFileName, compiledProgram);
                }
                
                if (compiledProgram.empty())
                {
#ifdef _DEBUG
					auto debug = String::create(L".%v.hlsl", hash);
					String debugFileName(FileSystem::ReplaceExtension(getContext()->getFileName(L"data\\cache", name), debug));
					FileSystem::Save(debugFileName, uncompiledProgram);
#endif
					compiledProgram = videoDevice->compileProgram(pipelineType, name, uncompiledProgram, entryFunction);
                    FileSystem::Save(cacheFileName, compiledProgram);
                }

                return compiledProgram;
            }

            ProgramHandle loadProgram(Video::PipelineType pipelineType, const wchar_t *name, const wchar_t *entryFunction, const wchar_t *engineData)
            {
                GEK_REQUIRE(name);
                GEK_REQUIRE(entryFunction);
                GEK_REQUIRE(engineData);

                auto load = [this, pipelineType, name = String(name), entryFunction = String(entryFunction), engineData = String(engineData)](ProgramHandle)->Video::ObjectPtr
                {
                    auto compiledProgram = compileProgram(pipelineType, name, entryFunction, engineData);
                    auto program = videoDevice->createProgram(pipelineType, compiledProgram.data(), compiledProgram.size());
                    program->setName(String::create(L"%v:%v", name, entryFunction));
                    return program;
                };

                return programCache.getHandle(std::move(load));
            }

            RenderStateHandle createRenderState(const Video::RenderStateInformation &renderState)
            {
                auto load = [this, renderState](RenderStateHandle) -> Video::ObjectPtr
                {
                    auto state = videoDevice->createRenderState(renderState);
					//state->setName(stateName);
					return state;
                };

                auto hash = GetHash(static_cast<uint8_t>(renderState.fillMode),
                    static_cast<uint8_t>(renderState.cullMode),
                    renderState.frontCounterClockwise,
                    renderState.depthBias,
                    renderState.depthBiasClamp,
                    renderState.slopeScaledDepthBias,
                    renderState.depthClipEnable,
                    renderState.scissorEnable,
                    renderState.multisampleEnable,
                    renderState.antialiasedLineEnable);
                return renderStateCache.getHandle(hash, std::move(load));
            }

            DepthStateHandle createDepthState(const Video::DepthStateInformation &depthState)
            {
                auto load = [this, depthState](DepthStateHandle) -> Video::ObjectPtr
                {
					auto state = videoDevice->createDepthState(depthState);
					//state->setName(stateName);
					return state;
				};

                auto hash = GetHash(depthState.enable,
                    static_cast<uint8_t>(depthState.writeMask),
                    static_cast<uint8_t>(depthState.comparisonFunction),
                    depthState.stencilEnable,
                    depthState.stencilReadMask,
                    depthState.stencilWriteMask,
                    static_cast<uint8_t>(depthState.stencilFrontState.failOperation),
                    static_cast<uint8_t>(depthState.stencilFrontState.depthFailOperation),
                    static_cast<uint8_t>(depthState.stencilFrontState.passOperation),
                    static_cast<uint8_t>(depthState.stencilFrontState.comparisonFunction),
                    static_cast<uint8_t>(depthState.stencilBackState.failOperation),
                    static_cast<uint8_t>(depthState.stencilBackState.depthFailOperation),
                    static_cast<uint8_t>(depthState.stencilBackState.passOperation),
                    static_cast<uint8_t>(depthState.stencilBackState.comparisonFunction));
                return depthStateCache.getHandle(hash, std::move(load));
            }

            BlendStateHandle createBlendState(const Video::UnifiedBlendStateInformation &blendState)
            {
                auto load = [this, blendState](BlendStateHandle) -> Video::ObjectPtr
                {
					auto state = videoDevice->createBlendState(blendState);
					//state->setName(stateName);
					return state;
				};

                auto hash = GetHash(blendState.enable,
                    static_cast<uint8_t>(blendState.colorSource),
                    static_cast<uint8_t>(blendState.colorDestination),
                    static_cast<uint8_t>(blendState.colorOperation),
                    static_cast<uint8_t>(blendState.alphaSource),
                    static_cast<uint8_t>(blendState.alphaDestination),
                    static_cast<uint8_t>(blendState.alphaOperation),
                    blendState.writeMask);
                return blendStateCache.getHandle(hash, std::move(load));
            }

            BlendStateHandle createBlendState(const Video::IndependentBlendStateInformation &blendState)
            {
                auto load = [this, blendState](BlendStateHandle) -> Video::ObjectPtr
                {
                    auto state = videoDevice->createBlendState(blendState);
					//state->setName(stateName);
					return state;
				};

                auto hash = 0;
                for (uint32_t renderTarget = 0; renderTarget < 8; ++renderTarget)
                {
                    if (blendState.targetStates[renderTarget].enable)
                    {
                        hash = GetHash(hash, renderTarget,
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorSource),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorDestination),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorOperation),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaSource),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaDestination),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaOperation),
                            blendState.targetStates[renderTarget].writeMask);
                    }
                }

                return blendStateCache.getHandle(hash, std::move(load));
            }

            void generateMipMaps(Video::Device::Context *videoContext, ResourceHandle resourceHandle)
            {
                GEK_REQUIRE(videoContext);

                auto resource = dynamicCache.getResource(resourceHandle);
                if (resource)
                {
                    videoContext->generateMipMaps(dynamic_cast<Video::Texture *>(resource));
                }
            }

            void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle)
            {
                auto source = dynamicCache.getResource(sourceHandle);
                auto destination = dynamicCache.getResource(destinationHandle);
                if (source && destination)
                {
                    videoDevice->copyResource(destination, source);
                }
            }

            void clearUnorderedAccess(Video::Device::Context *videoContext, ResourceHandle resourceHandle, const Math::Float4 &value)
            {
                GEK_REQUIRE(videoContext);

                auto resource = dynamicCache.getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearUnorderedAccess(resource, value);
                }
            }

            void clearUnorderedAccess(Video::Device::Context *videoContext, ResourceHandle resourceHandle, const uint32_t value[4])
            {
                GEK_REQUIRE(videoContext);

                auto resource = dynamicCache.getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearUnorderedAccess(resource, value);
                }
            }

            void clearRenderTarget(Video::Device::Context *videoContext, ResourceHandle resourceHandle, const Math::Float4 &color)
            {
                GEK_REQUIRE(videoContext);

                auto resource = dynamicCache.getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearRenderTarget(dynamic_cast<Video::Target *>(resource), color);
                }
            }

            void clearDepthStencilTarget(Video::Device::Context *videoContext, ResourceHandle depthBufferHandle, uint32_t flags, float clearDepth, uint32_t clearStencil)
            {
                GEK_REQUIRE(videoContext);

                auto depthBuffer = dynamicCache.getResource(depthBufferHandle);
                if (depthBuffer)
                {
                    videoContext->clearDepthStencilTarget(depthBuffer, flags, clearDepth, clearStencil);
                }
            }

            void setMaterial(Video::Device::Context *videoContext, Engine::Shader::Pass *pass, MaterialHandle handle)
            {
                GEK_REQUIRE(videoContext);
                GEK_REQUIRE(pass);

                if (drawPrimitiveValid)
                {
                    auto material = materialCache.getResource(handle);
                    if (drawPrimitiveValid =( material != nullptr))
                    {
                        auto passData = material->getPassData(pass->getIdentifier());
                        if (drawPrimitiveValid = (passData != nullptr))
                        {
                            setRenderState(videoContext, passData->renderState);
                            setResourceList(videoContext->pixelPipeline(), passData->resourceList, pass->getFirstResourceStage());
                        }
                    }
                }
            }

            void setVisual(Video::Device::Context *videoContext, VisualHandle handle)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    auto visual = visualCache.getResource(handle);
                    if (drawPrimitiveValid = (visual != nullptr))
                    {
                        visual->enable(videoContext);
                    }
                }
            }

            void setRenderState(Video::Device::Context *videoContext, RenderStateHandle renderStateHandle)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    auto renderState = renderStateCache.getResource(renderStateHandle);
                    if (drawPrimitiveValid = (renderState != nullptr))
                    {
                        videoContext->setRenderState(renderState);
                    }
                }
            }

            void setDepthState(Video::Device::Context *videoContext, DepthStateHandle depthStateHandle, uint32_t stencilReference)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    auto depthState = depthStateCache.getResource(depthStateHandle);
                    if (drawPrimitiveValid = (depthState != nullptr))
                    {
                        videoContext->setDepthState(depthState, stencilReference);
                    }
                }
            }

            void setBlendState(Video::Device::Context *videoContext, BlendStateHandle blendStateHandle, const Math::Float4 &blendFactor, uint32_t sampleMask)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    auto blendState = blendStateCache.getResource(blendStateHandle);
                    if (drawPrimitiveValid = (blendState != nullptr))
                    {
                        videoContext->setBlendState(blendState, blendFactor, sampleMask);
                    }
                }
            }

            void setProgram(Video::Device::Context::Pipeline *videoPipeline, ProgramHandle programHandle)
            {
                GEK_REQUIRE(videoPipeline);

                auto &valid = getValid(videoPipeline);
                if (valid)
                {
                    auto program = programCache.getResource(programHandle);
                    if (valid && (valid = (program != nullptr)))
                    {
                        videoPipeline->setProgram(program);
                    }
                }
            }

            ObjectCache<Video::Target> renderTargetCache;
            std::vector<Video::ViewPort> viewPortCache;
            void setRenderTargetList(Video::Device::Context *videoContext, const std::vector<ResourceHandle> &renderTargetHandleList, ResourceHandle *depthBuffer)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid && (drawPrimitiveValid = renderTargetCache.set(renderTargetHandleList, dynamicCache)))
                {
                    auto &renderTargetList = renderTargetCache.get();
                    uint32_t renderTargetCount = renderTargetList.size();
                    viewPortCache.resize(renderTargetCount);
                    for (uint32_t renderTarget = 0; renderTarget < renderTargetCount; ++renderTarget)
                    {
                        viewPortCache[renderTarget] = renderTargetList[renderTarget]->getViewPort();
                    }

                    videoContext->setRenderTargetList(renderTargetList, (depthBuffer ? dynamicCache.getResource(*depthBuffer) : nullptr));
                    videoContext->setViewportList(viewPortCache);
                }
            }

            void setBackBuffer(Video::Device::Context *videoContext, ResourceHandle *depthBuffer)
            {
                GEK_REQUIRE(videoContext);
                
                auto &renderTargetList = renderTargetCache.get();
                renderTargetList.resize(1);
                renderTargetList[0] = videoDevice->getBackBuffer();
                videoContext->setRenderTargetList(renderTargetList, (depthBuffer ? dynamicCache.getResource(*depthBuffer) : nullptr));

                viewPortCache.resize(1);
                viewPortCache[0] = renderTargetList[0]->getViewPort();
                videoContext->setViewportList(viewPortCache);
            }

            void clearRenderTargetList(Video::Device::Context *videoContext, int32_t count, bool depthBuffer)
            {
                GEK_REQUIRE(videoContext);

                videoContext->clearRenderTargetList(count, depthBuffer);
            }

            void startResourceBlock(void)
            {
                drawPrimitiveValid = true;
                dispatchValid = true;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Resources);
    }; // namespace Implementation
}; // namespace Gek
