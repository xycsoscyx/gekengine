#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Shapes/Sphere.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Visual.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Shader.hpp"
#include "GEK/Engine/Filter.hpp"
#include "GEK/Engine/Material.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Component.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Light.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Utility/ContextUser.hpp"
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
            virtual ~ResourceRequester(void) = default;

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
            uint32_t validationIdentifier = 0;
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
                validationIdentifier = nextIdentifier;
                resourceHandleMap.clear();
                resourceMap.clear();
            }

            void setResource(HANDLE handle, const TypePtr &data)
            {
                auto &resource = resourceMap.insert(std::make_pair(handle, nullptr)).first->second;
                std::atomic_exchange(&resource, data);
            }

            virtual TYPE * const getResource(HANDLE handle) const
            {
                if (handle.identifier >= validationIdentifier)
                {
                    auto resourceSearch = resourceMap.find(handle);
                    if (resourceSearch != std::end(resourceMap))
                    {
                        return std::atomic_load(&resourceSearch->second).get();
                    }
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

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> &&load)
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return std::make_pair(false, resourceSearch->second);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    resources->addRequest([this, handle, load = move(load)](void) -> void
                    {
                        setResource(handle, load(handle));
                    });

                    return std::make_pair(true, handle);
                }

                return std::make_pair(false, HANDLE());
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

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::size_t parameters, std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        HANDLE handle = resourceSearch->second;
                        auto loadParametersSearch = loadParameters.find(handle);
                        if (loadParametersSearch == std::end(loadParameters) || loadParametersSearch->second != parameters)
                        {
                            loadParameters[handle] = parameters;
                            resources->addRequest([this, handle, load = move(load)](void) -> void
                            {
                                setResource(handle, load(handle));
                            });
                        }

                        return std::make_pair(false, handle);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    loadParameters[handle] = parameters;
                    resources->addRequest([this, handle, load = move(load)](void) -> void
                    {
                        setResource(handle, load(handle));
                    });

                    return std::make_pair(true, handle);
                }

                return std::make_pair(false, HANDLE());
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
                    auto &resource = std::atomic_load(&resourceSearch.second);
                    if (resource)
                    {
                        try
                        {
                            resource->reload();
                        }
                        catch (...)
                        {
                        };
                    }
                }
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                ResourceCache::clear();
            }

            std::pair<bool, HANDLE> getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != std::end(resourceHandleMap))
                    {
                        return std::make_pair(false, resourceSearch->second);
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    HANDLE handle = getNextHandle();
                    resourceHandleMap[hash] = handle;
                    try
                    {
                        setResource(handle, load(handle));
                        return std::make_pair(true, handle);
                    }
                    catch (...)
                    {
                    };
                }

                return std::make_pair(false, HANDLE());
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
                const auto listCount = inputList.size();
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
                const auto listCount = inputList.size();
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

        GEK_CONTEXT_USER(Resources, Plugin::Core *)
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
            concurrency::concurrent_unordered_map<ResourceHandle, Video::Texture::Description> textureDescriptionMap;
            concurrency::concurrent_unordered_map<ResourceHandle, Video::Buffer::Description> bufferDescriptionMap;

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

        public:
            Resources(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , videoDevice(core->getVideoDevice())
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

                loadPool.drain();
                core->onResize.disconnect<Resources, &Resources::onResize>(this);
            }

            Validate &getValid(Video::Device::Context::Pipeline *videoPipeline)
            {
                GEK_REQUIRE(videoPipeline);

                return (videoPipeline->getType() == Video::PipelineType::Compute ? dispatchValid : drawPrimitiveValid);
            }

            Video::TexturePtr loadTextureData(FileSystem::Path const &filePath, WString const &textureName, uint32_t flags)
            {
                auto texture = videoDevice->loadTexture(filePath, flags);
                texture->setName(textureName);
                return texture;
            }

            WString getFullProgram(WString const &name, WString const &engineData)
            {
                auto programsPath(getContext()->getRootFileName(L"data", L"programs"));
                auto filePath(FileSystem::GetFileName(programsPath, name));
                if (filePath.isFile())
                {
                    WString programDirectory(filePath.getParentPath());

					WString baseProgram(FileSystem::Load(filePath, CString::Empty));
                    baseProgram.replace(L"\r", L"$");
                    baseProgram.replace(L"\n", L"$");
                    baseProgram.replace(L"$$", L"$");
                    auto programLines = baseProgram.split(L'$', false);

                    WString uncompiledProgram;
                    for (const auto &line : programLines)
                    {
                        if (line.empty())
                        {
                            uncompiledProgram.append(L"\r\n");
                        }
                        else if (line.find(L"#include") == 0)
                        {
                            WString includeName(line.subString(8));
                            includeName.trim();

                            if (includeName.empty())
                            {
                                throw InvalidIncludeName("Empty include encountered");
                            }
                            else
                            {
                                WString includeData;
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
                                        auto localPath(FileSystem::GetFileName(programDirectory, includeName));
                                        if (localPath.isFile())
                                        {
											includeData = FileSystem::Load(localPath, CString::Empty);
                                        }
                                    }
                                    else if (includeType == L'<')
                                    {
                                        auto rootPath(FileSystem::GetFileName(programsPath, includeName));
                                        if (rootPath.isFile())
                                        {
											includeData = FileSystem::Load(rootPath, CString::Empty);
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
                Video::Texture::Description description;
                description.format = Video::Format::R11G11B10_FLOAT;
                description.width = backBuffer->getDescription().width;
                description.height = backBuffer->getDescription().height;
                description.flags = Video::Texture::Description::Flags::RenderTarget | Video::Texture::Description::Flags::Resource;
                createTexture(L"screen", description);
                createTexture(L"screenBuffer", description);
            }

            // ResourceRequester
            void addRequest(std::function<void(void)> &&load)
            {
                loadPool.enqueue([this, load = move(load)](void) -> void
                {
                    try
                    {
                        load();
                    }
                    catch (const std::exception &exception)
                    {
                        core->getLog()->message("Resources", Plugin::Core::Log::Type::Error, "Error occurred trying to load an external resource: %v", exception.what());
                    }
                    catch (...)
                    {
                        core->getLog()->message("Resources", Plugin::Core::Log::Type::Error, "Unknown error occurred trying to load an external resource");
                    };
                });
            }

            // Plugin::Resources
            VisualHandle loadVisual(WString const &visualName)
            {
                auto load = [this, visualName](VisualHandle)->Plugin::VisualPtr
                {
                    return getContext()->createClass<Plugin::Visual>(L"Engine::Visual", videoDevice, (Engine::Resources *)this, visualName);
                };

                auto hash = GetHash(visualName);
                return visualCache.getHandle(hash, std::move(load)).second;
            }

            MaterialHandle loadMaterial(WString const &materialName)
            {
                auto load = [this, materialName](MaterialHandle handle)->Engine::MaterialPtr
                {
                    return getContext()->createClass<Engine::Material>(L"Engine::Material", (Engine::Resources *)this, materialName, handle);
                };

                auto hash = GetHash(materialName);
                return materialCache.getHandle(hash, std::move(load)).second;
            }

            ResourceHandle loadTexture(WString const &textureName, uint32_t flags)
            {
                // iterate over formats in case the texture name has no extension
                static const WString formatList[] =
                {
                    L"",
                    L".dds",
                    L".tga",
                    L".png",
                    L".jpg",
                    L".bmp",
                };

                auto texturePath(getContext()->getRootFileName(L"data", L"textures", textureName));
                for (const auto &format : formatList)
                {
                    auto filePath(texturePath.withExtension(format));
                    if (filePath.isFile())
                    {
                        auto load = [this, filePath = FileSystem::Path(filePath), textureName = WString(textureName), flags](ResourceHandle)->Video::TexturePtr
                        {
                            return loadTextureData(filePath, textureName, flags);
                        };

                        auto hash = GetHash(textureName);
                        auto resource = dynamicCache.getHandle(hash, flags, std::move(load));
                        if (resource.first)
                        {
                            auto description = videoDevice->loadTextureDescription(filePath);
                            textureDescriptionMap.insert(std::make_pair(resource.second, description));
                        }

                        return resource.second;
                    }
                }

                return ResourceHandle();
            }

            ResourceHandle createPattern(WString const &pattern, const JSON::Object &parameters)
            {
                std::vector<uint8_t> data;
                Video::Texture::Description description;
                if (pattern.compareNoCase(L"color") == 0)
                {
                    try
                    {
                        if (parameters.is_array())
                        {
                            switch (parameters.size())
                            {
                            case 1:
                                data.push_back(parameters.at(0).as_uint());
                                description.format = Video::Format::R8_UNORM;
                                break;

                            case 2:
                                data.push_back(parameters.at(0).as_uint());
                                data.push_back(parameters.at(1).as_uint());
                                description.format = Video::Format::R8G8_UNORM;
                                break;

                            case 3:
                                data.push_back(parameters.at(0).as_uint());
                                data.push_back(parameters.at(1).as_uint());
                                data.push_back(parameters.at(2).as_uint());
                                data.push_back(0);
                                description.format = Video::Format::R8G8B8A8_UNORM;
                                break;

                            case 4:
                                data.push_back(parameters.at(0).as_uint());
                                data.push_back(parameters.at(1).as_uint());
                                data.push_back(parameters.at(2).as_uint());
                                data.push_back(parameters.at(3).as_uint());
                                description.format = Video::Format::R8G8B8A8_UNORM;
                                break;
                            };
                        }
                        else if (parameters.is<float>())
                        {
                            data.push_back(uint8_t(parameters.as<float>() * 255.0f));
                            description.format = Video::Format::R8_UNORM;
                        }
                        else
                        {
                            data.push_back(parameters.as_uint());
                            description.format = Video::Format::R8_UNORM;
                        }
                    }
                    catch (...)
                    {
                        throw InvalidParameter("Unable to determine color texture type");
                    };
                }
                else if (pattern.compareNoCase(L"normal") == 0)
                {
                    Math::Float3 normal(Math::Float3::Zero);
                    if (parameters.is_array() && parameters.size() == 3)
                    {
                        normal.x = parameters.at(0).as<float>();
                        normal.x = parameters.at(1).as<float>();
                        normal.x = parameters.at(2).as<float>();
                    }

                    data.push_back(uint8_t(((normal.x + 1.0f) * 0.5f) * 255.0f));
                    data.push_back(uint8_t(((normal.y + 1.0f) * 0.5f) * 255.0f));
                    data.push_back(uint8_t(((normal.z + 1.0f) * 0.5f) * 255.0f));
                    data.push_back(255);

                    description.format = Video::Format::R8G8B8A8_UNORM;
                }
                else if (pattern.compareNoCase(L"system") == 0)
                {
                    if (parameters.is_string())
                    {
                        WString type(parameters.as_cstring());
                        if (type.compareNoCase(L"debug") == 0)
                        {
                            data.push_back(255);
                            data.push_back(0);
                            data.push_back(255);
                            data.push_back(255);
                            description.format = Video::Format::R8G8B8A8_UNORM;
                        }
                        else if (type.compareNoCase(L"flat") == 0)
                        {
                            Math::Float3 normal(0.0f, 0.0f, 1.0f);
                            uint8_t normalData[4] =
                            {
                                uint8_t(((normal.x + 1.0f) * 0.5f) * 255.0f),
                                uint8_t(((normal.y + 1.0f) * 0.5f) * 255.0f),
                                uint8_t(((normal.z + 1.0f) * 0.5f) * 255.0f),
                                255,
                            };

                            description.format = Video::Format::R8G8B8A8_UNORM;
                        }
                        else
                        {
                            throw InvalidParameter("Unknown system pattern encountered");
                        }
                    }
                    else
                    {
                        throw InvalidParameter("Unknown pattern attribute encountered");
                    }
                }
                else
                {
                    throw InvalidParameter("Unknown texture pattern encountered");
                }

                if (description.format == Video::Format::Unknown)
                {
                    throw InvalidParameter("Invalid color format encountered");
                }

                WString name(WString::Format(L"%v:%v", pattern, parameters.to_string()));
                description.flags = Video::Texture::Description::Flags::Resource;
                auto load = [this, name, description, data = move(data)](ResourceHandle) mutable -> Video::TexturePtr
                {
                    auto texture = videoDevice->createTexture(description, data.data());
                    texture->setName(name);
                    return texture;
                };

                auto hash = GetHash(name);
                auto resource = dynamicCache.getHandle(hash, 0, std::move(load));
                if (resource.first)
                {
                    textureDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createTexture(WString const &textureName, const Video::Texture::Description &description)
            {
                auto load = [this, textureName, description](ResourceHandle)->Video::TexturePtr
                {
                    auto texture = videoDevice->createTexture(description);
                    texture->setName(textureName);
                    return texture;
                };

                auto hash = GetHash(textureName);
                auto parameters = description.getHash();
                auto resource = dynamicCache.getHandle(hash, parameters, std::move(load));
                if (resource.first)
                {
                    textureDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createBuffer(WString const &bufferName, const Video::Buffer::Description &description)
            {
                GEK_REQUIRE(description.count > 0);

                auto load = [this, bufferName, description](ResourceHandle)->Video::BufferPtr
                {
                    auto buffer = videoDevice->createBuffer(description);
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto hash = GetHash(bufferName);
                auto parameters = description.getHash();
                auto resource = dynamicCache.getHandle(hash, parameters, std::move(load));
                if (resource.first)
                {
                    bufferDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
            }

            ResourceHandle createBuffer(WString const &bufferName, const Video::Buffer::Description &description, std::vector<uint8_t> &&staticData)
            {
                GEK_REQUIRE(description.count > 0);
                GEK_REQUIRE(!staticData.empty());

                auto load = [this, bufferName, description, staticData = move(staticData)](ResourceHandle)->Video::BufferPtr
                {
                    auto buffer = videoDevice->createBuffer(description, (void *)staticData.data());
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto hash = GetHash(bufferName);
                auto parameters = reinterpret_cast<std::size_t>(staticData.data());
                auto resource = dynamicCache.getHandle(hash, parameters, std::move(load));
                if (resource.first)
                {
                    bufferDescriptionMap.insert(std::make_pair(resource.second, description));
                }

                return resource.second;
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
                    core->getLog()->adjustValue("Draw", "Primitive Count", 1.0f);
                    core->getLog()->adjustValue("Draw", "Vertex Count", vertexCount);
                    videoContext->drawPrimitive(vertexCount, firstVertex);
                }
            }

            void drawInstancedPrimitive(Video::Device::Context *videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
            {
                if (drawPrimitiveValid)
                {
                    core->getLog()->adjustValue("Draw", "Primitive Count", 1.0f);
                    core->getLog()->adjustValue("Draw", "Vertex Count", vertexCount);
                    core->getLog()->adjustValue("Draw", "Instance Count", instanceCount);
                    videoContext->drawInstancedPrimitive(instanceCount, firstInstance, vertexCount, firstVertex);
                }
            }

            void drawIndexedPrimitive(Video::Device::Context *videoContext, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    core->getLog()->adjustValue("Draw", "Primitive Count", 1.0f);
                    core->getLog()->adjustValue("Draw", "Index Count", indexCount);
                    videoContext->drawIndexedPrimitive(indexCount, firstIndex, firstVertex);
                }
            }

            void drawInstancedIndexedPrimitive(Video::Device::Context *videoContext, uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
            {
                GEK_REQUIRE(videoContext);

                if (drawPrimitiveValid)
                {
                    core->getLog()->adjustValue("Draw", "Primitive Count", 1.0f);
                    core->getLog()->adjustValue("Draw", "Index Count", indexCount);
                    core->getLog()->adjustValue("Draw", "Instance Count", instanceCount);
                    videoContext->drawInstancedIndexedPrimitive(instanceCount, firstInstance, indexCount, firstIndex, firstVertex);
                }
            }

            void dispatch(Video::Device::Context *videoContext, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
            {
                GEK_REQUIRE(videoContext);

                if (dispatchValid)
                {
                    core->getLog()->adjustValue("Draw", "Dispatch Count", 1.0f);
                    core->getLog()->adjustValue("Draw", "Thread Count", threadGroupCountX * threadGroupCountY * threadGroupCountZ);
                    videoContext->dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                }
            }

            // Engine::Resources
            void clear(void)
            {
                textureDescriptionMap.clear();
                bufferDescriptionMap.clear();
                loadPool.reset();
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
                Video::Texture::Description description;
                description.format = Video::Format::R11G11B10_FLOAT;
                description.width = backBuffer->getDescription().width;
                description.height = backBuffer->getDescription().height;
                description.flags = Video::Texture::Description::Flags::RenderTarget | Video::Texture::Description::Flags::Resource;
                createTexture(L"screen", description);
                createTexture(L"screenBuffer", description);
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

            ResourceHandle getResourceHandle(WString const &resourceName) const
            {
                return dynamicCache.getHandle(GetHash(resourceName));
            }

            Engine::Shader * const getShader(ShaderHandle handle) const
            {
                return shaderCache.getResource(handle);
            }

            Engine::Shader * const getShader(WString const &shaderName, MaterialHandle material)
            {
                std::unique_lock<std::recursive_mutex> lock(shaderMutex);
                auto load = [this, shaderName](ShaderHandle) -> Engine::ShaderPtr
                {
                    return getContext()->createClass<Engine::Shader>(L"Engine::Shader", core->getLog(), videoDevice, (Engine::Resources *)this, core->getPopulation(), shaderName);
                };

                auto hash = GetHash(shaderName);
                auto resource = shaderCache.getHandle(hash, std::move(load));
                if (material && resource.second)
                {
                    materialShaderMap[material] = resource.second;
                }

                return shaderCache.getResource(resource.second);
            }

            Engine::Filter * const getFilter(WString const &filterName)
            {
                auto load = [this, filterName](ResourceHandle)->Engine::FilterPtr
                {
                    return getContext()->createClass<Engine::Filter>(L"Engine::Filter", core->getLog(), videoDevice, (Engine::Resources *)this, core->getPopulation(), filterName);
                };

                auto hash = GetHash(filterName);
                auto resource = filterCache.getHandle(hash, std::move(load));
                return filterCache.getResource(resource.second);
            }

            Video::Texture::Description * const getTextureDescription(ResourceHandle resourceHandle)
            {
                auto descriptionSearch = textureDescriptionMap.find(resourceHandle);
                if (descriptionSearch != std::end(textureDescriptionMap))
                {
                    return &descriptionSearch->second;
                }
                else
                {
                    return nullptr;
                }
            }

            Video::Buffer::Description * const getBufferDescription(ResourceHandle resourceHandle)
            {
                auto descriptionSearch = bufferDescriptionMap.find(resourceHandle);
                if (descriptionSearch != std::end(bufferDescriptionMap))
                {
                    return &descriptionSearch->second;
                }
                else
                {
                    return nullptr;
                }
            }

            std::vector<uint8_t> compileProgram(Video::PipelineType pipelineType, WString const &name, WString const &entryFunction, WString const &engineData)
            {
                auto uncompiledProgram = getFullProgram(name, engineData);

                auto hash = GetHash(uncompiledProgram);
                auto cacheExtension = WString::Format(L".%v.bin", hash);
                auto cachePath(getContext()->getRootFileName(L"data", L"cache", name).withExtension(cacheExtension));

				std::vector<uint8_t> compiledProgram;
                if (cachePath.isFile())
                {
					static const std::vector<uint8_t> EmptyBuffer;
					compiledProgram = FileSystem::Load(cachePath, EmptyBuffer);
                }
                
                if (compiledProgram.empty())
                {
#ifdef _DEBUG
					auto debugExtension = WString::Format(L".%v.hlsl", hash);
					WString debugPath(getContext()->getRootFileName(L"data", L"cache", name).withExtension(debugExtension));
					FileSystem::Save(debugPath, CString(uncompiledProgram));
#endif
					compiledProgram = videoDevice->compileProgram(pipelineType, name, uncompiledProgram, entryFunction);
                    FileSystem::Save(cachePath, compiledProgram);
                }

                return compiledProgram;
            }

            ProgramHandle loadProgram(Video::PipelineType pipelineType, WString const &name, WString const &entryFunction, WString const &engineData)
            {
                auto load = [this, pipelineType, name, entryFunction, engineData](ProgramHandle)->Video::ObjectPtr
                {
                    auto compiledProgram = compileProgram(pipelineType, name, entryFunction, engineData);
                    auto program = videoDevice->createProgram(pipelineType, compiledProgram.data(), compiledProgram.size());
                    program->setName(WString::Format(L"%v:%v", name, entryFunction));
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

                auto hash = renderState.getHash();
                return renderStateCache.getHandle(hash, std::move(load)).second;
            }

            DepthStateHandle createDepthState(const Video::DepthStateInformation &depthState)
            {
                auto load = [this, depthState](DepthStateHandle) -> Video::ObjectPtr
                {
					auto state = videoDevice->createDepthState(depthState);
					//state->setName(stateName);
					return state;
				};

                auto hash = depthState.getHash();
                return depthStateCache.getHandle(hash, std::move(load)).second;
            }

            BlendStateHandle createBlendState(const Video::UnifiedBlendStateInformation &blendState)
            {
                auto load = [this, blendState](BlendStateHandle) -> Video::ObjectPtr
                {
					auto state = videoDevice->createBlendState(blendState);
					//state->setName(stateName);
					return state;
				};

                auto hash = blendState.getHash();
                return blendStateCache.getHandle(hash, std::move(load)).second;
            }

            BlendStateHandle createBlendState(const Video::IndependentBlendStateInformation &blendState)
            {
                auto load = [this, blendState](BlendStateHandle) -> Video::ObjectPtr
                {
                    auto state = videoDevice->createBlendState(blendState);
					//state->setName(stateName);
					return state;
				};

                auto hash = blendState.getHash();
                return blendStateCache.getHandle(hash, std::move(load)).second;
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

            void resolveSamples(Video::Device::Context *videoContext, ResourceHandle destinationHandle, ResourceHandle sourceHandle)
            {
                auto source = dynamicCache.getResource(sourceHandle);
                auto destination = dynamicCache.getResource(destinationHandle);
                if (source && destination)
                {
                    videoContext->resolveSamples(dynamic_cast<Video::Texture *>(destination), dynamic_cast<Video::Texture *>(source));
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

            void clearUnorderedAccess(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::Float4 const &value)
            {
                GEK_REQUIRE(videoContext);

                auto resource = dynamicCache.getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearUnorderedAccess(resource, value);
                }
            }

            void clearUnorderedAccess(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::UInt4 const &value)
            {
                GEK_REQUIRE(videoContext);

                auto resource = dynamicCache.getResource(resourceHandle);
                if (resource)
                {
                    videoContext->clearUnorderedAccess(resource, value);
                }
            }

            void clearRenderTarget(Video::Device::Context *videoContext, ResourceHandle resourceHandle, Math::Float4 const &color)
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

            void setBlendState(Video::Device::Context *videoContext, BlendStateHandle blendStateHandle, Math::Float4 const &blendFactor, uint32_t sampleMask)
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
                    const uint32_t renderTargetCount = renderTargetList.size();
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
