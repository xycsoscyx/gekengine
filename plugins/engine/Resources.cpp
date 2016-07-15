#include "GEK\Utility\String.h"
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
#include <ppl.h>
#include <future>
#include <set>

namespace Gek
{
    namespace Implementation
    {
        static ShuntingYard shuntingYard;

        GEK_INTERFACE(RequestLoader)
        {
            virtual void request(std::function<void(void)> load, bool threaded) = 0;
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

            struct ReadWriteResource
            {
                AtomicResource read;
                AtomicResource write;

                ReadWriteResource(void)
                {
                }

                void setRead(TypePtr data)
                {
                    this->read.set(data);
                }

                void setWrite(TypePtr data)
                {
                    this->write.set(data);
                }
            };

        private:
            RequestLoader *loader;

            uint64_t nextIdentifier;
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;
            concurrency::concurrent_unordered_map<std::size_t, HANDLE> resourceHandleMap;
            concurrency::concurrent_unordered_map<HANDLE, AtomicResource> localResourceMap;
            concurrency::concurrent_unordered_map<HANDLE, ReadWriteResource> readWriteResourceMap;
            concurrency::concurrent_unordered_map<HANDLE, AtomicResource> globalResourceMap;

        public:
            ResourceManager(RequestLoader *loader)
                : loader(loader)
                , nextIdentifier(0)
            {
            }

            ~ResourceManager(void)
            {
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                resourceHandleMap.clear();
                localResourceMap.clear();
                readWriteResourceMap.clear();
            }

            HANDLE getGlobalHandle(std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
            {
                HANDLE handle;
                handle.assign(InterlockedIncrement(&nextIdentifier));
                loader->request([this, handle, requestLoad = move(requestLoad)](void) -> void
                {
                    requestLoad(handle, [this, handle](TypePtr data) -> void
                    {
                        auto &resource = globalResourceMap[handle];
                        resource.set(data);
                    });
                }, false);

                return handle;
            }

            HANDLE getUniqueHandle(std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
            {
                HANDLE handle;
                handle.assign(InterlockedIncrement(&nextIdentifier));
                loader->request([this, handle, requestLoad = move(requestLoad)](void) -> void
                {
                    requestLoad(handle, [this, handle](TypePtr data) -> void
                    {
                        auto &resource = localResourceMap[handle];
                        resource.set(data);
                    });
                }, false);

                return handle;
            }

            HANDLE getUniqueReadWriteHandle(std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
            {
                HANDLE handle;
                handle.assign(InterlockedIncrement(&nextIdentifier));
                loader->request([this, handle, requestLoad = move(requestLoad)](void) -> void
                {
                    requestLoad(handle, [this, handle](TypePtr data) -> void
                    {
                        auto &resource = readWriteResourceMap[handle];
                        resource.setRead(data);
                    });

                    requestLoad(handle, [this, handle](TypePtr data) -> void
                    {
                        auto &resource = readWriteResourceMap[handle];
                        resource.setWrite(data);
                    });
                }, false);

                return handle;
            }

            HANDLE getHandle(std::size_t hash, std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad, std::function<void(TYPE *)> onLoaded, bool threaded)
            {
                HANDLE handle;
                if (requestedLoadSet.count(hash) > 0)
                {
                    auto resourceSearch = resourceHandleMap.find(hash);
                    if (resourceSearch != resourceHandleMap.end())
                    {
                        handle = resourceSearch->second;
                        if (onLoaded)
                        {
                            onLoaded(getResource(handle, false));
                        }
                    }
                }
                else
                {
                    requestedLoadSet.insert(hash);
                    handle.assign(InterlockedIncrement(&nextIdentifier));
                    resourceHandleMap[hash] = handle;
                    loader->request([this, handle, requestLoad = move(requestLoad)](void) -> void
                    {
                        requestLoad(handle, [this, handle](TypePtr data) -> void
                        {
                            auto &resource = localResourceMap[handle];
                            resource.set(data);
                        });
                    }, threaded);
                }

                return handle;
            }

            HANDLE getReadWriteHandle(std::size_t hash, std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad, bool threaded)
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
                    loader->request([this, handle, requestLoad = move(requestLoad)](void) -> void
                    {
                        requestLoad(handle, [this, handle](TypePtr data) -> void
                        {
                            auto &resource = readWriteResourceMap[handle];
                            resource.setRead(data);
                        });

                        requestLoad(handle, [this, handle](TypePtr data) -> void
                        {
                            auto &resource = readWriteResourceMap[handle];
                            resource.setWrite(data);
                        });
                    }, threaded);
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

            TYPE * const getResource(HANDLE handle, bool writable = false) const
            {
                auto globalSearch = globalResourceMap.find(handle);
                if (globalSearch != globalResourceMap.end())
                {
                    return globalSearch->second.get();
                }

                auto localSearch = localResourceMap.find(handle);
                if (localSearch != localResourceMap.end())
                {
                    return localSearch->second.get();
                }

                auto readWriteSearch = readWriteResourceMap.find(handle);
                if (readWriteSearch != readWriteResourceMap.end())
                {
                    auto &resource = readWriteSearch->second;
                    return (writable ? resource.write.get() : resource.read.get());
                }

                return nullptr;
            }

            void flipResource(HANDLE handle)
            {
                auto readWriteSearch = readWriteResourceMap.find(handle);
                if (readWriteSearch != readWriteResourceMap.end())
                {
                    auto &resource = readWriteSearch->second;
                    if (resource.read.state == AtomicResource::State::Loaded &&
                        resource.write.state == AtomicResource::State::Loaded)
                    {
                        std::swap(resource.read.data, resource.write.data);
                    }
                }
            }
        };

        GEK_CONTEXT_USER(Resources, Plugin::Core *, Video::Device *)
            , public Engine::Resources
            , public RequestLoader
        {
        private:
            Plugin::Core *core;
            Video::Device *device;

            ResourceManager<ProgramHandle, Video::Object> programManager;
            ResourceManager<VisualHandle, Plugin::Visual> visualManager;
            ResourceManager<MaterialHandle, Engine::Material> materialManager;
            ResourceManager<ShaderHandle, Engine::Shader> shaderManager;
            ResourceManager<ResourceHandle, Engine::Filter> filterManager;
            ResourceManager<ResourceHandle, Video::Object> resourceManager;
            ResourceManager<RenderStateHandle, Video::Object> renderStateManager;
            ResourceManager<DepthStateHandle, Video::Object> depthStateManager;
            ResourceManager<BlendStateHandle, Video::Object> blendStateManager;

            concurrency::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

            std::future<void> loadResourceRunning;
            concurrency::concurrent_queue<std::function<void(void)>> loadResourceQueue;

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
            }

            // RequestLoader
            void tryLoadingRequest(std::function<void(void)> load)
            {
                try
                {
                    load();
                }
                catch (const Exception &)
                {
                };
            }

            void request(std::function<void(void)> load, bool threaded)
            {
                threaded = false;
                if (threaded)
                {
                    loadResourceQueue.push(load);
                    if (!loadResourceRunning.valid() || (loadResourceRunning.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
                    {
                        loadResourceRunning = std::async(std::launch::async, [this](void) -> void
                        {
                            CoInitialize(nullptr);
                            std::function<void(void)> load;
                            while (loadResourceQueue.try_pop(load))
                            {
                                tryLoadingRequest(load);
                            };

                            CoUninitialize();
                        });
                    }
                }
                else
                {
                    tryLoadingRequest(load);
                }
            }

            // Plugin::Resources
            void clearLocal(void)
            {
                loadResourceQueue.clear();
                if (loadResourceRunning.valid() && (loadResourceRunning.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready))
                {
                    loadResourceRunning.get();
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
                auto ShaderSearch = materialShaderMap.find(material);
                if (ShaderSearch != materialShaderMap.end())
                {
                    return (*ShaderSearch).second;
                }

                return ShaderHandle();
            }

            ResourceHandle getResourceHandle(const wchar_t *resourceName) const
            {
                std::size_t hash = std::hash<String>()(resourceName);
                return resourceManager.getHandle(hash);
            }

            Engine::Shader * const getShader(ShaderHandle handle) const
            {
                return shaderManager.getResource(handle, false);
            }

            Plugin::Visual * const getVisual(VisualHandle handle) const
            {
                return visualManager.getResource(handle, false);
            }

            Engine::Material * const getMaterial(MaterialHandle handle) const
            {
                return materialManager.getResource(handle, false);
            }

            Video::Texture * const getTexture(ResourceHandle handle) const
            {
                return dynamic_cast<Video::Texture *>(resourceManager.getResource(handle));
            }

            VisualHandle loadPlugin(const wchar_t *pluginName)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(pluginName));
                auto load = [this, pluginName = String(pluginName)](VisualHandle handle)->Plugin::VisualPtr
                {
                    return getContext()->createClass<Plugin::Visual>(L"Engine::Visual", device, pluginName.c_str());
                };

                auto request = [this, load = std::move(load)](VisualHandle handle, std::function<void(Plugin::VisualPtr)> set) -> void
                {
                    set(load(handle));
                };

                return visualManager.getGlobalHandle(request);
            }

            MaterialHandle loadMaterial(const wchar_t *materialName)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(materialName));
                auto load = [this, materialName = String(materialName)](MaterialHandle handle)->Engine::MaterialPtr
                {
                    return getContext()->createClass<Engine::Material>(L"Engine::Material", (Engine::Resources *)this, materialName.c_str(), handle);
                };

                auto request = [this, load = std::move(load)](MaterialHandle handle, std::function<void(Engine::MaterialPtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = std::hash<String>()(materialName);
                return materialManager.getHandle(hash, request, nullptr, true);
            }

            Engine::Filter * const loadFilter(const wchar_t *filterName)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(filterName));
                auto load = [this, filterName = String(filterName)](ResourceHandle handle)->Engine::FilterPtr
                {
                    return getContext()->createClass<Engine::Filter>(L"Engine::Filter", device, (Engine::Resources *)this, filterName.c_str());
                };

                auto request = [this, load = std::move(load)](ResourceHandle handle, std::function<void(Engine::FilterPtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = std::hash<String>()(filterName);
                ResourceHandle filter = filterManager.getHandle(hash, request, nullptr, false);
                return filterManager.getResource(filter);
            }

            ShaderHandle loadShader(const wchar_t *shaderName, MaterialHandle material, std::function<void(Engine::Shader *)> onLoad)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(shaderName));
                auto load = [this, shaderName = String(shaderName), onLoad](ShaderHandle handle)->Engine::ShaderPtr
                {
                    Engine::ShaderPtr shader(getContext()->createClass<Engine::Shader>(L"Engine::Shader", device, (Engine::Resources *)this, core->getPopulation(), shaderName.c_str()));
                    onLoad(shader.get());
                    return shader;
                };

                auto request = [this, load = std::move(load)](ShaderHandle handle, std::function<void(Engine::ShaderPtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = std::hash<String>()(shaderName);
                ShaderHandle shader = shaderManager.getHandle(hash, request, onLoad, true);
                materialShaderMap[material] = shader;
                return shader;
            }

            RenderStateHandle createRenderState(const Video::RenderStateInformation &renderState)
            {
                GEK_TRACE_FUNCTION();
                auto load = [this, renderState](RenderStateHandle handle) -> Video::ObjectPtr
                {
                    return device->createRenderState(renderState);
                };

                auto request = [this, load = std::move(load)](RenderStateHandle handle, std::function<void(Video::ObjectPtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = std::hash_combine(static_cast<uint8_t>(renderState.fillMode),
                    static_cast<uint8_t>(renderState.cullMode),
                    renderState.frontCounterClockwise,
                    renderState.depthBias,
                    renderState.depthBiasClamp,
                    renderState.slopeScaledDepthBias,
                    renderState.depthClipEnable,
                    renderState.scissorEnable,
                    renderState.multisampleEnable,
                    renderState.antialiasedLineEnable);
                return renderStateManager.getHandle(hash, request, nullptr, true);
            }

            DepthStateHandle createDepthState(const Video::DepthStateInformation &depthState)
            {
                GEK_TRACE_FUNCTION();
                auto load = [this, depthState](DepthStateHandle handle) -> Video::ObjectPtr
                {
                    return device->createDepthState(depthState);
                };

                auto request = [this, load = std::move(load)](DepthStateHandle handle, std::function<void(Video::ObjectPtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = std::hash_combine(depthState.enable,
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
                return depthStateManager.getHandle(hash, request, nullptr, true);
            }

            BlendStateHandle createBlendState(const Video::UnifiedBlendStateInformation &blendState)
            {
                GEK_TRACE_FUNCTION();
                auto load = [this, blendState](BlendStateHandle handle) -> Video::ObjectPtr
                {
                    return device->createBlendState(blendState);
                };

                auto request = [this, load = std::move(load)](BlendStateHandle handle, std::function<void(Video::ObjectPtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = std::hash_combine(blendState.enable,
                    static_cast<uint8_t>(blendState.colorSource),
                    static_cast<uint8_t>(blendState.colorDestination),
                    static_cast<uint8_t>(blendState.colorOperation),
                    static_cast<uint8_t>(blendState.alphaSource),
                    static_cast<uint8_t>(blendState.alphaDestination),
                    static_cast<uint8_t>(blendState.alphaOperation),
                    blendState.writeMask);
                return blendStateManager.getHandle(hash, request, nullptr, true);
            }

            BlendStateHandle createBlendState(const Video::IndependentBlendStateInformation &blendState)
            {
                GEK_TRACE_FUNCTION();
                auto load = [this, blendState](BlendStateHandle handle) -> Video::ObjectPtr
                {
                    return device->createBlendState(blendState);
                };

                auto request = [this, load = std::move(load)](BlendStateHandle handle, std::function<void(Video::ObjectPtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = 0;
                for (uint32_t renderTarget = 0; renderTarget < 8; ++renderTarget)
                {
                    if (blendState.targetStates[renderTarget].enable)
                    {
                        std::hash_combine(hash, std::hash_combine(renderTarget,
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorSource),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorDestination),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].colorOperation),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaSource),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaDestination),
                            static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaOperation),
                            blendState.targetStates[renderTarget].writeMask));
                    }
                }

                return blendStateManager.getHandle(hash, request, nullptr, true);
            }

            ResourceHandle createTexture(const wchar_t *textureName, Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t flags, bool readWrite)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(textureName));
                auto load = [this, textureName = String(textureName), format, width, height, depth, mipmaps, flags](ResourceHandle handle) -> Video::TexturePtr
                {
                    auto texture = device->createTexture(format, width, height, depth, mipmaps, flags);
                    texture->setName(textureName);
                    return texture;
                };

                auto request = [this, load = std::move(load)](ResourceHandle handle, std::function<void(Video::TexturePtr)> set) -> void
                {
                    set(load(handle));
                };

                if (textureName)
                {
                    std::size_t hash = std::hash<String>()(textureName);
                    if (readWrite)
                    {
                        return resourceManager.getReadWriteHandle(hash, request, true);
                    }
                    else
                    {
                        return resourceManager.getHandle(hash, request, nullptr, true);
                    }
                }
                else
                {
                    return resourceManager.getGlobalHandle(request);
                }
            }

            ResourceHandle createBuffer(const wchar_t *bufferName, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, bool readWrite, const void *staticData)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(bufferName));
                auto load = [this, bufferName = String(bufferName), stride, count, type, flags, staticData](ResourceHandle handle) -> Video::BufferPtr
                {
                    auto buffer = device->createBuffer(stride, count, type, flags, staticData);
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto request = [this, load = std::move(load)](ResourceHandle handle, std::function<void(Video::BufferPtr)> set) -> void
                {
                    set(load(handle));
                };

                if (bufferName)
                {
                    std::size_t hash = std::hash<String>()(bufferName);
                    if (readWrite)
                    {
                        return resourceManager.getReadWriteHandle(hash, request, staticData ? false : true);
                    }
                    else
                    {
                        return resourceManager.getHandle(hash, request, nullptr, staticData ? false : true);
                    }
                }
                else
                {
                    return resourceManager.getGlobalHandle(request);
                }
            }

            ResourceHandle createBuffer(const wchar_t *bufferName, Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, bool readWrite, const void *staticData)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(bufferName));
                auto load = [this, bufferName = String(bufferName), format, count, type, flags, staticData](ResourceHandle handle) -> Video::BufferPtr
                {
                    auto buffer = device->createBuffer(format, count, type, flags, staticData);
                    buffer->setName(bufferName);
                    return buffer;
                };

                auto request = [this, load = std::move(load)](ResourceHandle handle, std::function<void(Video::BufferPtr)> set) -> void
                {
                    set(load(handle));
                };

                if (bufferName)
                {
                    std::size_t hash = std::hash<String>()(bufferName);
                    if (readWrite)
                    {
                        return resourceManager.getReadWriteHandle(hash, request, staticData ? false : true);
                    }
                    else
                    {
                        return resourceManager.getHandle(hash, request, nullptr, staticData ? false : true);
                    }
                }
                else
                {
                    return resourceManager.getGlobalHandle(request);
                }
            }

            Video::TexturePtr loadTextureData(const String &textureName, uint32_t flags)
            {
                // iterate over formats in case the texture name has no extension
                static const wchar_t *formatList[] =
                {
                    L"",
                    L".dds",
                    L".tga",
                    L".png",
                    L".jpg",
                    L".bmp",
                };

                for (auto &format : formatList)
                {
                    FileSystem::Path filePath(FileSystem::expandPath(String(L"$root\\data\\textures\\%v%v", textureName, format)));
                    if (filePath.isFile())
                    {
                        auto texture = device->loadTexture(filePath, flags);
                        texture->setName(filePath);
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
                    try
                    {
                        auto rpnTokenList = shuntingYard.getTokenList(parameters);
                        uint32_t colorSize = shuntingYard.getReturnSize(rpnTokenList);
                        if (colorSize == 1)
                        {
                            float color;
                            shuntingYard.evaluate(rpnTokenList, color);
                            uint8_t colorData[4] =
                            {
                                uint8_t(color * 255.0f),
                            };

                            texture = device->createTexture(Video::Format::R8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, colorData);
                        }
                        else if (colorSize == 2)
                        {
                            Math::Float2 color;
                            shuntingYard.evaluate(rpnTokenList, color);
                            uint8_t colorData[4] =
                            {
                                uint8_t(color.x * 255.0f),
                                uint8_t(color.y * 255.0f),
                            };

                            texture = device->createTexture(Video::Format::R8G8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, colorData);
                        }
                        else if (colorSize == 3)
                        {
                            Math::Float3 color;
                            shuntingYard.evaluate(rpnTokenList, color);
                            uint8_t colorData[4] =
                            {
                                uint8_t(color.x * 255.0f),
                                uint8_t(color.y * 255.0f),
                                uint8_t(color.z * 255.0f),
                                255,
                            };

                            texture = device->createTexture(Video::Format::R8G8B8A8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, colorData);
                        }
                        else if (colorSize == 4)
                        {
                            Math::Float4 color;
                            shuntingYard.evaluate(rpnTokenList, color);
                            uint8_t colorData[4] =
                            {
                                uint8_t(color.x * 255.0f),
                                uint8_t(color.y * 255.0f),
                                uint8_t(color.z * 255.0f),
                                uint8_t(color.w * 255.0f),
                            };

                            texture = device->createTexture(Video::Format::R8G8B8A8_UNORM, 1, 1, 1, 1, Video::TextureFlags::Resource, colorData);
                        }
                        else
                        {
                            GEK_THROW_EXCEPTION(Exception, "Invalid color size specified: %v", colorSize)
                        }
                    }
                    catch (const ShuntingYard::Exception &)
                    {
                    };
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
                        GEK_THROW_EXCEPTION(Exception, "Invalid system pattern specified: %v", parameters);
                    }
                }
                else
                {
                    GEK_THROW_EXCEPTION(Exception, "Invalid dynamic pattern specified: %v", pattern);
                }

                texture->setName(String(L"%v:%v", pattern, parameters));
                return texture;
            }

            ResourceHandle loadTexture(const wchar_t *textureName, uint32_t flags)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(textureName));
                auto load = [this, textureName = String(textureName), flags](ResourceHandle handle)->Video::TexturePtr
                {
                    return loadTextureData(textureName, flags);
                };

                auto request = [this, load = std::move(load)](ResourceHandle handle, std::function<void(Video::TexturePtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = std::hash<String>()(textureName);
                return resourceManager.getHandle(hash, request, nullptr, true);
            }

            ResourceHandle createTexture(const wchar_t *pattern, const wchar_t *parameters)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(parameters));

                auto load = [this, pattern = String(pattern), parameters = String(parameters)](ResourceHandle handle)->Video::TexturePtr
                {
                    return createTextureData(pattern, parameters);
                };

                auto request = [this, load = std::move(load)](ResourceHandle handle, std::function<void(Video::TexturePtr)> set) -> void
                {
                    set(load(handle));
                };

                std::size_t hash = std::hash_combine<String, String>(pattern, parameters);
                return resourceManager.getHandle(hash, request, nullptr, true);
            }

            ProgramHandle loadComputeProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude, const std::unordered_map<StringUTF8, StringUTF8> &defineList)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName), GEK_PARAMETER(entryFunction));
                auto load = [this, fileName = String(fileName), entryFunction = StringUTF8(entryFunction), onInclude = move(onInclude), defineList](ProgramHandle handle)->Video::ObjectPtr
                {
                    auto program = device->loadComputeProgram(fileName, entryFunction, onInclude, defineList);
                    program->setName(String(L"%v:%v", fileName, entryFunction));
                    return program;
                };

                auto request = [this, load = std::move(load)](ProgramHandle handle, std::function<void(Video::ObjectPtr)> set) -> void
                {
                    set(load(handle));
                };

                return programManager.getUniqueHandle(request);
            }

            ProgramHandle loadPixelProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude, const std::unordered_map<StringUTF8, StringUTF8> &defineList)
            {
                GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName), GEK_PARAMETER(entryFunction));
                auto load = [this, fileName = String(fileName), entryFunction = StringUTF8(entryFunction), onInclude = move(onInclude), defineList](ProgramHandle handle)->Video::ObjectPtr
                {
                    auto program = device->loadPixelProgram(fileName, entryFunction, onInclude, defineList);
                    program->setName(String(L"%v:%v", fileName, entryFunction));
                    return program;
                };

                auto request = [this, load = std::move(load)](ProgramHandle handle, std::function<void(Video::ObjectPtr)> set) -> void
                {
                    set(load(handle));
                };

                return programManager.getUniqueHandle(request);
            }

            void mapBuffer(ResourceHandle buffer, void **data)
            {
                device->mapBuffer(dynamic_cast<Video::Buffer *>(resourceManager.getResource(buffer)), data);
            }

            void unmapBuffer(ResourceHandle buffer)
            {
                device->unmapBuffer(dynamic_cast<Video::Buffer *>(resourceManager.getResource(buffer)));
            }

            void flip(ResourceHandle resourceHandle)
            {
                resourceManager.flipResource(resourceHandle);
            }

            void generateMipMaps(Video::Device::Context *deviceContext, ResourceHandle resourceHandle)
            {
                deviceContext->generateMipMaps(dynamic_cast<Video::Texture *>(resourceManager.getResource(resourceHandle)));
            }

            void copyResource(ResourceHandle sourceHandle, ResourceHandle destinationHandle)
            {
                device->copyResource(resourceManager.getResource(sourceHandle), resourceManager.getResource(destinationHandle));
            }

            void setRenderState(Video::Device::Context *deviceContext, RenderStateHandle renderStateHandle)
            {
                deviceContext->setRenderState(renderStateManager.getResource(renderStateHandle));
            }

            void setDepthState(Video::Device::Context *deviceContext, DepthStateHandle depthStateHandle, uint32_t stencilReference)
            {
                deviceContext->setDepthState(depthStateManager.getResource(depthStateHandle), stencilReference);
            }

            void setBlendState(Video::Device::Context *deviceContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, uint32_t sampleMask)
            {
                deviceContext->setBlendState(blendStateManager.getResource(blendStateHandle), blendFactor, sampleMask);
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
                resourceCache.resize(std::max(resourceCount, resourceCache.size()));
                for (uint32_t resource = 0; resource < resourceCount; resource++)
                {
                    resourceCache[resource] = (resourceHandleList ? resourceManager.getResource(resourceHandleList[resource]) : nullptr);
                }

                deviceContextPipeline->setResourceList(resourceCache.data(), resourceCache.size(), firstStage);
            }

            std::vector<Video::Object *> unorderedAccessCache;
            void setUnorderedAccessList(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle *resourceHandleList, uint32_t resourceCount, uint32_t firstStage)
            {
                unorderedAccessCache.resize(std::max(resourceCount, unorderedAccessCache.size()));
                for (uint32_t resource = 0; resource < resourceCount; resource++)
                {
                    unorderedAccessCache[resource] = (resourceHandleList ? resourceManager.getResource(resourceHandleList[resource]) : nullptr);
                }

                deviceContextPipeline->setUnorderedAccessList(unorderedAccessCache.data(), unorderedAccessCache.size(), firstStage);
            }

            void setConstantBuffer(Video::Device::Context::Pipeline *deviceContextPipeline, ResourceHandle resourceHandle, uint32_t stage)
            {
                deviceContextPipeline->setConstantBuffer((resourceHandle ? dynamic_cast<Video::Buffer *>(resourceManager.getResource(resourceHandle)) : nullptr), stage);
            }

            void setProgram(Video::Device::Context::Pipeline *deviceContextPipeline, ProgramHandle programHandle)
            {
                deviceContextPipeline->setProgram(programManager.getResource(programHandle));
            }

            void setVertexBuffer(Video::Device::Context *deviceContext, uint32_t slot, ResourceHandle resourceHandle, uint32_t offset)
            {
                deviceContext->setVertexBuffer(slot, dynamic_cast<Video::Buffer *>(resourceManager.getResource(resourceHandle)), offset);
            }

            void setIndexBuffer(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, uint32_t offset)
            {
                deviceContext->setIndexBuffer(dynamic_cast<Video::Buffer *>(resourceManager.getResource(resourceHandle)), offset);
            }

            void clearUnorderedAccess(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const Math::Float4 &value)
            {
                deviceContext->clearUnorderedAccess(dynamic_cast<Video::Object *>(resourceManager.getResource(resourceHandle, true)), value);
            }

            void clearUnorderedAccess(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const uint32_t value[4])
            {
                deviceContext->clearUnorderedAccess(dynamic_cast<Video::Object *>(resourceManager.getResource(resourceHandle, true)), value);
            }

            void clearRenderTarget(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const Math::Color &color)
            {
                deviceContext->clearRenderTarget(dynamic_cast<Video::Target *>(resourceManager.getResource(resourceHandle, true)), color);
            }

            void clearDepthStencilTarget(Video::Device::Context *deviceContext, ResourceHandle depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
            {
                deviceContext->clearDepthStencilTarget(resourceManager.getResource(depthBuffer), flags, clearDepth, clearStencil);
            }

            std::vector<Video::ViewPort> viewPortCache;
            std::vector<Video::Target *> renderTargetCache;
            void setRenderTargets(Video::Device::Context *deviceContext, ResourceHandle *renderTargetHandleList, uint32_t renderTargetHandleCount, ResourceHandle *depthBuffer)
            {
                viewPortCache.resize(std::max(renderTargetHandleCount, viewPortCache.size()));
                renderTargetCache.resize(std::max(renderTargetHandleCount, renderTargetCache.size()));
                for (uint32_t renderTarget = 0; renderTarget < renderTargetHandleCount; renderTarget++)
                {
                    renderTargetCache[renderTarget] = (renderTargetHandleList ? dynamic_cast<Video::Target *>(resourceManager.getResource(renderTargetHandleList[renderTarget], true)) : nullptr);
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
