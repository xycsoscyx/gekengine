﻿#include "GEK\Utility\String.h"
#include "GEK\Utility\ShuntingYard.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Visual.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Filter.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Light.h"
#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUser.h"
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <ppl.h>
#include <future>
#include <set>

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
        class ResourceManager
        {
        public:
            using TypePtr = std::shared_ptr<TYPE>;

            struct AtomicResource
            {
                enum class State : uint8_t
                {
                    Empty = 0,
                    Loading,
                    Loaded,
                };

                std::atomic<State> state;
                TypePtr data;

                AtomicResource(void)
                    : state(State::Empty)
                {
                }

                AtomicResource(const AtomicResource &resource)
                    : state(resource.state == State::Empty ? State::Empty : resource.state == State::Loading ? State::Loading : State::Loaded)
                    , data(resource.data)
                {
                }

                void clear(void)
                {
                    state = State::Empty;
                    this->data = nullptr;
                }

                void set(TypePtr data)
                {
                    if (state == State::Empty)
                    {
                        state = State::Loading;
                        this->data = data;
                        state = State::Loaded;
                    }
                }

                TYPE * const get(void) const
                {
                    return (state == State::Loaded ? data.get() : nullptr);
                }
            };

        protected:
            ResourceRequester *resources;

            uint64_t nextIdentifier;
            concurrency::concurrent_unordered_map<std::size_t, HANDLE> resourceHandleMap;
            concurrency::concurrent_unordered_map<HANDLE, AtomicResource> resourceMap;

        public:
            ResourceManager(ResourceRequester *resources)
                : resources(resources)
                , nextIdentifier(0)
            {
                GEK_REQUIRE(resources);
            }

            virtual ~ResourceManager(void)
            {
            }

            virtual void clear(void)
            {
                resourceHandleMap.clear();
                resourceMap.clear();
            }

            virtual TYPE * const getResource(HANDLE handle) const
            {
                auto resourceSearch = resourceMap.find(handle);
                if (resourceSearch != resourceMap.end())
                {
                    return resourceSearch->second.get();
                }

                return nullptr;
            }
        };

        template <class HANDLE, typename TYPE>
        class GeneralResourceManager
            : public ResourceManager<HANDLE, TYPE>
        {
        private:
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;

        public:
            GeneralResourceManager(ResourceRequester *resources)
                : ResourceManager(resources)
            {
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                ResourceManager::clear();
            }

            HANDLE getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != resourceHandleMap.end())
                    {
                        handle = resourceSearch->second;
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    handle.assign(InterlockedIncrement(&nextIdentifier));
                    resourceHandleMap[hash] = handle;
                    resources->addRequest([this, handle, load = move(load)](void) -> void
                    {
                        auto &resource = resourceMap[handle];
                        resource.set(load(handle));
                    });
                }

                return handle;
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != resourceHandleMap.end())
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        template <class HANDLE, typename TYPE>
        class ProgramResourceManager
            : public ResourceManager<HANDLE, TYPE>
        {
        public:
            ProgramResourceManager(ResourceRequester *resources)
                : ResourceManager(resources)
            {
            }

            HANDLE getHandle(std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                handle.assign(InterlockedIncrement(&nextIdentifier));
                resources->addRequest([this, handle, load = move(load)](void) -> void
                {
                    auto &resource = resourceMap[handle];
                    resource.set(load(handle));
                });

                return handle;
            }
        };

        template <class HANDLE, typename TYPE>
        class ShaderResourceManager
            : public ResourceManager<HANDLE, TYPE>
        {
        private:
            concurrency::concurrent_unordered_map<HANDLE, std::function<TypePtr(HANDLE)>> resourceLoadMap;
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;

        public:
            ShaderResourceManager(ResourceRequester *resources)
                : ResourceManager(resources)
            {
            }

            void wipe(void)
            {
                for (auto &resource : resourceMap)
                {
                    resource.second.clear();
                }
            }

            void reload(void)
            {
                for (auto &resource : resourceMap)
                {
                    auto loadSearch = resourceLoadMap.find(resource.first);
                    if (loadSearch != resourceLoadMap.end())
                    {
                        resource.second.set(loadSearch->second(resource.first));
                    }
                }
            }

            void clear(void)
            {
                resourceLoadMap.clear();
                requestedLoadSet.clear();
                ResourceManager::clear();
            }

            HANDLE getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> &&load)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != resourceHandleMap.end())
                    {
                        handle = resourceSearch->second;
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    handle.assign(InterlockedIncrement(&nextIdentifier));
                    resourceHandleMap[hash] = handle;

                    resourceLoadMap[handle] = load;
                    auto &resource = resourceMap[handle];
                    resource.set(load(handle));
                }

                return handle;
            }

            HANDLE getHandle(std::size_t hash) const
            {
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != resourceHandleMap.end())
                    {
                        return resourceSearch->second;
                    }
                }

                return HANDLE();
            }
        };

        GEK_CONTEXT_USER(Resources, Plugin::Core *, Video::Device *)
            , public Plugin::CoreListener
            , public Engine::Resources
            , public ResourceRequester
        {
        private:
            Plugin::Core *core;
            Video::Device *device;

            std::mutex loadMutex;
            std::future<void> loadThread;
            concurrency::concurrent_queue<std::function<void(void)>> loadQueue;

            ProgramResourceManager<ProgramHandle, Video::Object> programManager;
            GeneralResourceManager<VisualHandle, Plugin::Visual> visualManager;
            GeneralResourceManager<MaterialHandle, Engine::Material> materialManager;
            ShaderResourceManager<ShaderHandle, Engine::Shader> shaderManager;
            GeneralResourceManager<ResourceHandle, Engine::Filter> filterManager;
            GeneralResourceManager<ResourceHandle, Video::Object> resourceManager;
            GeneralResourceManager<RenderStateHandle, Video::Object> renderStateManager;
            GeneralResourceManager<DepthStateHandle, Video::Object> depthStateManager;
            GeneralResourceManager<BlendStateHandle, Video::Object> blendStateManager;

            concurrency::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

        public:
            Resources(Context *context, Plugin::Core *core, Video::Device *device)
                : ContextRegistration(context)
                , core(core)
                , device(device)
                , programManager(this)
                , visualManager(this)
                , materialManager(this)
                , shaderManager(this)
                , filterManager(this)
                , resourceManager(this)
                , renderStateManager(this)
                , depthStateManager(this)
                , blendStateManager(this)
            {
                GEK_REQUIRE(core);
                GEK_REQUIRE(device);
                core->addListener(this);
            }

            ~Resources(void)
            {
                loadQueue.clear();
                if (loadThread.valid() && loadThread.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    loadThread.get();
                }
            }

            // Plugin::CoreListener
            void onBeforeResize(void)
            {
                filterManager.clear();
                shaderManager.wipe();
            }

            virtual void onAfterResize(void)
            {
                shaderManager.reload();
            }

            // ResourceRequester
            void addRequest(std::function<void(void)> &&load)
            {
				loadQueue.push(std::move(load));
                std::lock_guard<std::mutex> lock(loadMutex);
                if (!loadThread.valid() || loadThread.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    loadThread = Gek::asynchronous([this](void) -> void
                    {
                        CoInitialize(nullptr);
                        std::function<void(void)> load;
                        while (loadQueue.try_pop(load))
                        {
                            try
                            {
                                load();
                            }
                            catch (...)
                            {
                            };
                        };

                        CoUninitialize();
                    });
                }
            }

            // Plugin::Resources
            void clearLocal(void)
            {
                loadQueue.clear();
                if (loadThread.valid() && loadThread.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    loadThread.get();
                }

                materialShaderMap.clear();
                programManager.clear();
                materialManager.clear();
                shaderManager.clear();
                filterManager.clear();
                resourceManager.clear();
                renderStateManager.clear();
                depthStateManager.clear();
                blendStateManager.clear();
            }

            ShaderHandle getMaterialShader(MaterialHandle material) const
            {
                auto shaderSearch = materialShaderMap.find(material);
                if (shaderSearch != materialShaderMap.end())
                {
                    return shaderSearch->second;
                }

                return ShaderHandle();
            }

            ResourceHandle getResourceHandle(const wchar_t *resourceName) const
            {
				return resourceManager.getHandle(getHash(resourceName));
            }

            Engine::Shader * const getShader(ShaderHandle handle) const
            {
                return shaderManager.getResource(handle);
            }

            Plugin::Visual * const getVisual(VisualHandle handle) const
            {
                return visualManager.getResource(handle);
            }

            Engine::Material * const getMaterial(MaterialHandle handle) const
            {
                return materialManager.getResource(handle);
            }

            Video::Texture * const getTexture(ResourceHandle handle) const
            {
                return dynamic_cast<Video::Texture *>(resourceManager.getResource(handle));
            }

            VisualHandle loadVisual(const wchar_t *visualName)
            {
                auto load = [this, visualName = String(visualName)](VisualHandle) -> Plugin::VisualPtr
                {
                    return getContext()->createClass<Plugin::Visual>(L"Engine::Visual", device, visualName);
                };

                auto hash = getHash(visualName);
                return visualManager.getHandle(hash, std::move(load));
            }

            MaterialHandle loadMaterial(const wchar_t *materialName)
            {
                auto load = [this, materialName = String(materialName)](MaterialHandle handle) -> Engine::MaterialPtr
                {
                    return getContext()->createClass<Engine::Material>(L"Engine::Material", (Engine::Resources *)this, materialName, handle);
                };

                auto hash = getHash(materialName);
                return materialManager.getHandle(hash, std::move(load));
            }

            Engine::Filter * const getFilter(const wchar_t *filterName)
            {
                auto load = [this, filterName = String(filterName)](ResourceHandle) -> Engine::FilterPtr
                {
                    return getContext()->createClass<Engine::Filter>(L"Engine::Filter", device, (Engine::Resources *)this, filterName);
                };

                auto hash = getHash(filterName);
                ResourceHandle filter = filterManager.getHandle(hash, std::move(load));
                return filterManager.getResource(filter);
            }

            Engine::Shader * const getShader(const wchar_t *shaderName, MaterialHandle material)
            {
                auto load = [this, shaderName = String(shaderName)](ShaderHandle) mutable -> Engine::ShaderPtr
                {
                    return getContext()->createClass<Engine::Shader>(L"Engine::Shader", device, (Engine::Resources *)this, core->getPopulation(), shaderName);
                };

                auto hash = getHash(shaderName);
                ShaderHandle shader = shaderManager.getHandle(hash, std::move(load));
                if (material)
                {
                    materialShaderMap[material] = shader;
                }

                return shaderManager.getResource(shader);
            }

            RenderStateHandle createRenderState(const Video::RenderStateInformation &renderState)
            {
                auto load = [this, renderState](RenderStateHandle) -> Video::ObjectPtr
                {
                    return device->createRenderState(renderState);
                };

                auto hash = getHash(static_cast<uint8_t>(renderState.fillMode),
                    static_cast<uint8_t>(renderState.cullMode),
                    renderState.frontCounterClockwise,
                    renderState.depthBias,
                    renderState.depthBiasClamp,
                    renderState.slopeScaledDepthBias,
                    renderState.depthClipEnable,
                    renderState.scissorEnable,
                    renderState.multisampleEnable,
                    renderState.antialiasedLineEnable);
                return renderStateManager.getHandle(hash, std::move(load));
            }

            DepthStateHandle createDepthState(const Video::DepthStateInformation &depthState)
            {
                auto load = [this, depthState](DepthStateHandle) -> Video::ObjectPtr
                {
                    return device->createDepthState(depthState);
                };

                auto hash = getHash(depthState.enable,
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
                return depthStateManager.getHandle(hash, std::move(load));
            }

            BlendStateHandle createBlendState(const Video::UnifiedBlendStateInformation &blendState)
            {
                auto load = [this, blendState](BlendStateHandle) -> Video::ObjectPtr
                {
                    return device->createBlendState(blendState);
                };

                auto hash = getHash(blendState.enable,
                    static_cast<uint8_t>(blendState.colorSource),
                    static_cast<uint8_t>(blendState.colorDestination),
                    static_cast<uint8_t>(blendState.colorOperation),
                    static_cast<uint8_t>(blendState.alphaSource),
                    static_cast<uint8_t>(blendState.alphaDestination),
                    static_cast<uint8_t>(blendState.alphaOperation),
                    blendState.writeMask);
                return blendStateManager.getHandle(hash, std::move(load));
            }

            BlendStateHandle createBlendState(const Video::IndependentBlendStateInformation &blendState)
            {
                auto load = [this, blendState](BlendStateHandle) -> Video::ObjectPtr
                {
                    return device->createBlendState(blendState);
                };

                auto hash = 0;
                for (uint32_t renderTarget = 0; renderTarget < 8; ++renderTarget)
                {
                    if (blendState.targetStates[renderTarget].enable)
                    {
                        hash = getHash(hash, renderTarget,
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorSource),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorDestination),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorOperation),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaSource),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaDestination),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaOperation),
                            blendState.targetStates[renderTarget].writeMask);
                    }
                }

                return blendStateManager.getHandle(hash, std::move(load));
            }

            ResourceHandle createTexture(const wchar_t *textureName, Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t flags)
            {
                auto load = [this, textureName = String(textureName), format, width, height, depth, mipmaps, flags](ResourceHandle) -> Video::TexturePtr
                {
                    auto texture = device->createTexture(format, width, height, depth, mipmaps, flags);
                    texture->setName(textureName);
                    return texture;
                };

                auto hash = getHash(textureName);
                return resourceManager.getHandle(hash, std::move(load));
            }

            ResourceHandle createBuffer(const wchar_t *bufferName, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const std::vector<uint8_t> &staticData)
            {
                auto load = [this, bufferName = String(bufferName), stride, count, type, flags, staticData](ResourceHandle) -> Video::BufferPtr
                {
                    auto buffer = device->createBuffer(stride, count, type, flags, (void *)staticData.data());
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto hash = getHash(bufferName);
                return resourceManager.getHandle(hash, std::move(load));
            }

            ResourceHandle createBuffer(const wchar_t *bufferName, Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const std::vector<uint8_t> &staticData)
            {
                auto load = [this, bufferName = String(bufferName), format, count, type, flags, staticData](ResourceHandle) -> Video::BufferPtr
                {
                    auto buffer = device->createBuffer(format, count, type, flags, (void *)staticData.data());
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto hash = getHash(bufferName);
                return resourceManager.getHandle(hash, std::move(load));
            }

            Video::TexturePtr loadTextureData(const wchar_t *textureName, uint32_t flags)
            {
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

					if (FileSystem::isFile(fileName))
                    {
                        auto texture = device->loadTexture(fileName, flags);
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
                        throw InvalidParameter();
                    }

                    texture = device->createTexture(format, 1, 1, 1, 1, Video::TextureFlags::Resource, colorData);
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

                    texture = device->createTexture(Video::Format::R8G8B8A8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, normalData);
                }
                else if (pattern.compareNoCase(L"system") == 0)
                {
                    if (parameters.compareNoCase(L"debug") == 0)
                    {
                        uint8_t data[] =
                        {
                            255, 0, 255, 255,
                        };

                        texture = device->createTexture(Video::Format::R8G8B8A8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, data);
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

                        texture = device->createTexture(Video::Format::R8G8B8A8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, normalData);
                    }
                    else
                    {
                        throw InvalidParameter();
                    }
                }
                else
                {
                    throw InvalidParameter();
                }

                texture->setName(String::create(L"%v:%v", pattern, parameters));
                return texture;
            }

            ResourceHandle loadTexture(const wchar_t *textureName, uint32_t flags)
            {
                auto load = [this, textureName = String(textureName), flags](ResourceHandle) -> Video::TexturePtr
                {
                    return loadTextureData(textureName, flags);
                };

                auto hash = getHash(textureName);
                return resourceManager.getHandle(hash, std::move(load));
            }

            ResourceHandle createTexture(const wchar_t *pattern, const wchar_t *parameters)
            {
                auto load = [this, pattern = String(pattern), parameters = String(parameters)](ResourceHandle) -> Video::TexturePtr
                {
                    return createTextureData(pattern, parameters);
                };

				auto hash = getHash(pattern, parameters);
                return resourceManager.getHandle(hash, std::move(load));
            }

            ProgramHandle loadComputeProgram(const wchar_t *fileName, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
            {
                auto load = [this, fileName = String(fileName), entryFunction = String(entryFunction), onInclude = onInclude](ProgramHandle) -> Video::ObjectPtr
                {
					String compiledFileName(FileSystem::replaceExtension(fileName, L".bin"));

					std::vector<uint8_t> compiled;
					if (false)//FileSystem::isFile(compiledFileName) && FileSystem::isFileNewer(compiledFileName, fileName))
					{
						FileSystem::load(compiledFileName, compiled);
					}
					else
					{
						String uncompiledProgram;
						FileSystem::load(fileName, uncompiledProgram);
						compiled = device->compileComputeProgram(fileName, uncompiledProgram, entryFunction, onInclude);
						FileSystem::save(compiledFileName, compiled);
					}

					auto program = device->createComputeProgram(compiled.data(), compiled.size());
                    program->setName(String::create(L"%v:%v", fileName, entryFunction));
                    return program;
                };

                return programManager.getHandle(std::move(load));
            }

            ProgramHandle loadPixelProgram(const wchar_t *fileName, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
            {
                auto load = [this, fileName = String(fileName), entryFunction = String(entryFunction), onInclude = onInclude](ProgramHandle) -> Video::ObjectPtr
				{
					String compiledFileName(FileSystem::replaceExtension(fileName, L".bin"));

					std::vector<uint8_t> compiled;
					if (false)// FileSystem::isFile(compiledFileName) && FileSystem::isFileNewer(compiledFileName, fileName))
					{
						FileSystem::load(compiledFileName, compiled);
					}
					else
					{
						String uncompiledProgram;
						FileSystem::load(fileName, uncompiledProgram);
						compiled = device->compilePixelProgram(fileName, uncompiledProgram, entryFunction, onInclude);
						FileSystem::save(compiledFileName, compiled);
					}

					auto program = device->createPixelProgram(compiled.data(), compiled.size());
					program->setName(String::create(L"%v:%v", fileName, entryFunction));
					return program;
                };

                return programManager.getHandle(std::move(load));
            }

            void mapBuffer(ResourceHandle bufferHandle, void **data)
            {
                GEK_REQUIRE(data);

                auto buffer = resourceManager.getResource(bufferHandle);
                if (buffer == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                device->mapBuffer(dynamic_cast<Video::Buffer *>(buffer), data);
            }

            void unmapBuffer(ResourceHandle bufferHandle)
            {
                auto buffer = resourceManager.getResource(bufferHandle);
                if (buffer == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                device->unmapBuffer(dynamic_cast<Video::Buffer *>(buffer));
            }

            void generateMipMaps(Video::Device::Context *deviceContext, ResourceHandle resourceHandle)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                if (resource == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->generateMipMaps(dynamic_cast<Video::Texture *>(resource));
            }

            void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle)
            {
                auto source = resourceManager.getResource(sourceHandle);
                auto destination = resourceManager.getResource(destinationHandle);
                if (source == nullptr || destination == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                device->copyResource(destination, source);
            }

            void setRenderState(Video::Device::Context *deviceContext, RenderStateHandle renderStateHandle)
            {
                GEK_REQUIRE(deviceContext);

                auto renderState = renderStateManager.getResource(renderStateHandle);
                if (renderState == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->setRenderState(renderState);
            }

            void setDepthState(Video::Device::Context *deviceContext, DepthStateHandle depthStateHandle, uint32_t stencilReference)
            {
                GEK_REQUIRE(deviceContext);

                auto depthState = depthStateManager.getResource(depthStateHandle);
                if (depthState == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->setDepthState(depthState, stencilReference);
            }

            void setBlendState(Video::Device::Context *deviceContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, uint32_t sampleMask)
            {
                GEK_REQUIRE(deviceContext);

                auto blendState = blendStateManager.getResource(blendStateHandle);
                if (blendState == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->setBlendState(blendState, blendFactor, sampleMask);
            }

            void setResource(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle resourceHandle, uint32_t stage)
            {
                setResourceList(deviceContextPipeline, &resourceHandle, 1, stage);
            }

            void setUnorderedAccess(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle resourceHandle, uint32_t stage)
            {
                setUnorderedAccessList(deviceContextPipeline, &resourceHandle, 1, stage);
            }

            std::vector<Video::Object *> resourceCache;
            void setResourceList(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle *resourceHandleList, uint32_t resourceCount, uint32_t firstStage)
            {
                GEK_REQUIRE(deviceContextPipeline);

                resourceCache.resize(std::max(resourceCount, resourceCache.size()));
                for (uint32_t resource = 0; resource < resourceCount; resource++)
                {
                    resourceCache[resource] = (resourceHandleList ? resourceManager.getResource(resourceHandleList[resource]) : nullptr);
                }

                deviceContextPipeline->setResourceList(resourceCache.data(), resourceCount, firstStage);
            }

            std::vector<Video::Object *> unorderedAccessCache;
            void setUnorderedAccessList(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle *resourceHandleList, uint32_t resourceCount, uint32_t firstStage)
            {
                GEK_REQUIRE(deviceContextPipeline);

                unorderedAccessCache.resize(std::max(resourceCount, unorderedAccessCache.size()));
                for (uint32_t resource = 0; resource < resourceCount; resource++)
                {
                    unorderedAccessCache[resource] = (resourceHandleList ? resourceManager.getResource(resourceHandleList[resource]) : nullptr);
                }

                deviceContextPipeline->setUnorderedAccessList(unorderedAccessCache.data(), resourceCount, firstStage);
            }

            void setConstantBuffer(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle resourceHandle, uint32_t stage)
            {
                GEK_REQUIRE(deviceContextPipeline);

                deviceContextPipeline->setConstantBuffer((resourceHandle ? dynamic_cast<Video::Buffer *>(resourceManager.getResource(resourceHandle)) : nullptr), stage);
            }

            void setProgram(Video::Device::Context::Pipeline *deviceContextPipeline, ProgramHandle programHandle)
            {
                GEK_REQUIRE(deviceContextPipeline);

                auto program = programManager.getResource(programHandle);
                if (program == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContextPipeline->setProgram(program);
            }

            void setVertexBuffer(Video::Device::Context *deviceContext, uint32_t slot, ResourceHandle resourceHandle, uint32_t offset)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                if (resource == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->setVertexBuffer(slot, dynamic_cast<Video::Buffer *>(resource), offset);
            }

            void setIndexBuffer(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, uint32_t offset)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                if (resource == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->setIndexBuffer(dynamic_cast<Video::Buffer *>(resource), offset);
            }

            void clearUnorderedAccess(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const Math::Float4 &value)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                if (resource == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->clearUnorderedAccess(resource, value);
            }

            void clearUnorderedAccess(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const uint32_t value[4])
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                if (resource == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->clearUnorderedAccess(resource, value);
            }

            void clearRenderTarget(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const Math::Color &color)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                if (resource == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->clearRenderTarget(dynamic_cast<Video::Target *>(resource), color);
            }

            void clearDepthStencilTarget(Video::Device::Context *deviceContext, ResourceHandle depthBufferHandle, uint32_t flags, float clearDepth, uint32_t clearStencil)
            {
                GEK_REQUIRE(deviceContext);

                auto depthBuffer = resourceManager.getResource(depthBufferHandle);
                if (depthBuffer == nullptr)
                {
                    throw ResourceNotLoaded();
                }

                deviceContext->clearDepthStencilTarget(depthBuffer, flags, clearDepth, clearStencil);
            }

            std::vector<Video::ViewPort> viewPortCache;
            std::vector<Video::Target *> renderTargetCache;
            void setRenderTargets(Video::Device::Context *deviceContext, ResourceHandle *renderTargetHandleList, uint32_t renderTargetHandleCount, ResourceHandle *depthBuffer)
            {
                GEK_REQUIRE(deviceContext);

                viewPortCache.resize(std::max(renderTargetHandleCount, viewPortCache.size()));
                renderTargetCache.resize(std::max(renderTargetHandleCount, renderTargetCache.size()));
                for (uint32_t renderTarget = 0; renderTarget < renderTargetHandleCount; renderTarget++)
                {
                    renderTargetCache[renderTarget] = (renderTargetHandleList ? dynamic_cast<Video::Target *>(resourceManager.getResource(renderTargetHandleList[renderTarget])) : nullptr);
                    if (renderTargetCache[renderTarget])
                    {
                        viewPortCache[renderTarget] = renderTargetCache[renderTarget]->getViewPort();
                    }
                }

                deviceContext->setRenderTargets(renderTargetCache.data(), renderTargetHandleCount, (depthBuffer ? resourceManager.getResource(*depthBuffer) : nullptr));
                if (renderTargetHandleList)
                {
                    deviceContext->setViewports(viewPortCache.data(), renderTargetHandleCount);
                }
            }

            void setBackBuffer(Video::Device::Context *deviceContext, ResourceHandle *depthBuffer)
            {
                GEK_REQUIRE(deviceContext);

                viewPortCache.resize(std::max(1U, viewPortCache.size()));
                renderTargetCache.resize(std::max(1U, renderTargetCache.size()));

                auto backBuffer = device->getBackBuffer();

                renderTargetCache[0] = backBuffer;
                deviceContext->setRenderTargets(renderTargetCache.data(), 1, (depthBuffer ? resourceManager.getResource(*depthBuffer) : nullptr));

                viewPortCache[0] = renderTargetCache[0]->getViewPort();
                deviceContext->setViewports(viewPortCache.data(), 1);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Resources);
    }; // namespace Implementation
}; // namespace Gek
