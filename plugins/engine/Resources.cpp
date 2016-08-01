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
#include <concurrent_vector.h>
#include <ppl.h>
#include <future>
#include <set>

namespace Gek
{
    namespace Implementation
    {
        static ShuntingYard shuntingYard;

        GEK_INTERFACE(Messenger)
        {
            virtual void addRequest(std::function<void(void)> load) = 0;
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

        private:
            Messenger *messenger;

            uint64_t nextIdentifier;
            concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;
            concurrency::concurrent_unordered_map<std::size_t, HANDLE> resourceHandleMap;
            concurrency::concurrent_unordered_map<HANDLE, AtomicResource> resourceMap;

        public:
            ResourceManager(Messenger *messenger)
                : messenger(messenger)
                , nextIdentifier(0)
            {
                GEK_REQUIRE(messenger);
            }

            ~ResourceManager(void)
            {
            }

            void clear(void)
            {
                requestedLoadSet.clear();
                resourceHandleMap.clear();
                resourceMap.clear();
            }

            HANDLE getUniqueHandle(std::function<TypePtr(HANDLE)> load)
            {
                HANDLE handle;
                handle.assign(InterlockedIncrement(&nextIdentifier));
                messenger->addRequest([this, handle, load = move(load)](void) -> void
                {
                    auto &resource = resourceMap[handle];
                    resource.set(load(handle));
                });

                return handle;
            }

            HANDLE getHandle(std::size_t hash, std::function<TypePtr(HANDLE)> load)
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
                    messenger->addRequest([this, handle, load = move(load)](void) -> void
                    {
                        auto &resource = resourceMap[handle];
                        resource.set(load(handle));
                    });
                }

                return handle;
            }

            HANDLE getImmediateHandle(std::size_t hash, std::function<TypePtr(HANDLE)> load)
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

            TYPE * const getResource(HANDLE handle) const
            {
                auto resourceSearch = resourceMap.find(handle);
                if (resourceSearch != resourceMap.end())
                {
                    return resourceSearch->second.get();
                }

                return nullptr;
            }
        };

        GEK_CONTEXT_USER(Resources, Plugin::Core *, Video::Device *)
            , public Engine::Resources
            , public Messenger
        {
        private:
            Plugin::Core *core;
            Video::Device *device;

            std::mutex loadMutex;
            std::future<void> loadThread;
            concurrency::concurrent_queue<std::function<void(void)>> loadQueue;

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
            }

            ~Resources(void)
            {
                loadQueue.clear();
                if (loadThread.valid() && loadThread.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    loadThread.get();
                }
            }

            // Messenger
            void addRequest(std::function<void(void)> load)
            {
                loadQueue.push(load);
                std::lock_guard<std::mutex> lock(loadMutex);
                if (!loadThread.valid() || loadThread.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
                {
                    loadThread = std::async(std::launch::async, [this](void) -> void
                    {
                        CoInitialize(nullptr);
                        std::function<void(void)> load;
                        while (loadQueue.try_pop(load))
                        {
                            try
                            {
                                load();
                            }
                            catch (const Gek::Exception &)
                            {
                            }
                            catch (...)
                            {
                                GEK_TRACE_EVENT("General exception occurred when loading resource");
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
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(visualName));
                    return getContext()->createClass<Plugin::Visual>(L"Engine::Visual", device, visualName.c_str());
                };

                std::size_t hash = std::hash<String>()(visualName);
                return visualManager.getHandle(hash, load);
            }

            MaterialHandle loadMaterial(const wchar_t *materialName)
            {
                auto load = [this, materialName = String(materialName)](MaterialHandle handle) -> Engine::MaterialPtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(materialName));
                    return getContext()->createClass<Engine::Material>(L"Engine::Material", (Engine::Resources *)this, materialName.c_str(), handle);
                };

                std::size_t hash = std::hash<String>()(materialName);
                return materialManager.getHandle(hash, load);
            }

            Engine::Filter * const getFilter(const wchar_t *filterName)
            {
                auto load = [this, filterName = String(filterName)](ResourceHandle) -> Engine::FilterPtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(filterName));
                    return getContext()->createClass<Engine::Filter>(L"Engine::Filter", device, (Engine::Resources *)this, filterName.c_str());
                };

                std::size_t hash = std::hash<String>()(filterName);
                ResourceHandle filter = filterManager.getHandle(hash, load);
                return filterManager.getResource(filter);
            }

            Engine::Shader * const getShader(const wchar_t *shaderName, MaterialHandle material)
            {
                auto load = [this, shaderName = String(shaderName)](ShaderHandle) -> Engine::ShaderPtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(shaderName));
                    if (!shaderName.empty() && shaderName.at(0) == L'$')
                    {
                    }

                    return getContext()->createClass<Engine::Shader>(L"Engine::Shader", device, (Engine::Resources *)this, core->getPopulation(), shaderName.c_str());
                };

                std::size_t hash = std::hash<String>()(shaderName);
                ShaderHandle shader = shaderManager.getImmediateHandle(hash, load);
                materialShaderMap[material] = shader;
                return shaderManager.getResource(shader);
            }

            RenderStateHandle createRenderState(const Video::RenderStateInformation &renderState)
            {
                auto load = [this, renderState](RenderStateHandle) -> Video::ObjectPtr
                {
                    GEK_TRACE_FUNCTION();
                    return device->createRenderState(renderState);
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
                return renderStateManager.getHandle(hash, load);
            }

            DepthStateHandle createDepthState(const Video::DepthStateInformation &depthState)
            {
                auto load = [this, depthState](DepthStateHandle) -> Video::ObjectPtr
                {
                    GEK_TRACE_FUNCTION();
                    return device->createDepthState(depthState);
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
                return depthStateManager.getHandle(hash, load);
            }

            BlendStateHandle createBlendState(const Video::UnifiedBlendStateInformation &blendState)
            {
                auto load = [this, blendState](BlendStateHandle) -> Video::ObjectPtr
                {
                    GEK_TRACE_FUNCTION();
                    return device->createBlendState(blendState);
                };

                std::size_t hash = std::hash_combine(blendState.enable,
                    static_cast<uint8_t>(blendState.colorSource),
                    static_cast<uint8_t>(blendState.colorDestination),
                    static_cast<uint8_t>(blendState.colorOperation),
                    static_cast<uint8_t>(blendState.alphaSource),
                    static_cast<uint8_t>(blendState.alphaDestination),
                    static_cast<uint8_t>(blendState.alphaOperation),
                    blendState.writeMask);
                return blendStateManager.getHandle(hash, load);
            }

            BlendStateHandle createBlendState(const Video::IndependentBlendStateInformation &blendState)
            {
                auto load = [this, blendState](BlendStateHandle) -> Video::ObjectPtr
                {
                    GEK_TRACE_FUNCTION();
                    return device->createBlendState(blendState);
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

                return blendStateManager.getHandle(hash, load);
            }

            ResourceHandle createTexture(const wchar_t *textureName, Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t flags)
            {
                auto load = [this, textureName = String(textureName), format, width, height, depth, mipmaps, flags](ResourceHandle) -> Video::TexturePtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(textureName), GEK_PARAMETER_TYPE(format, uint8_t), GEK_PARAMETER(width), GEK_PARAMETER(height), GEK_PARAMETER(depth), GEK_PARAMETER(mipmaps), GEK_PARAMETER(flags));
                    auto texture = device->createTexture(format, width, height, depth, mipmaps, flags);
                    texture->setName(textureName);
                    return texture;
                };

                std::size_t hash = std::hash<String>()(textureName);
                return resourceManager.getHandle(hash, load);
            }

            ResourceHandle createBuffer(const wchar_t *bufferName, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const std::vector<uint8_t> &staticData)
            {
                auto load = [this, bufferName = String(bufferName), stride, count, type, flags, staticData](ResourceHandle) -> Video::BufferPtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(bufferName), GEK_PARAMETER(stride), GEK_PARAMETER(count), GEK_PARAMETER_TYPE(type, uint8_t), GEK_PARAMETER(flags));
                    auto buffer = device->createBuffer(stride, count, type, flags, (void *)staticData.data());
                    buffer->setName(bufferName);
                    return buffer;
                };

                std::size_t hash = std::hash<String>()(bufferName);
                return resourceManager.getHandle(hash, load);
            }

            ResourceHandle createBuffer(const wchar_t *bufferName, Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const std::vector<uint8_t> &staticData)
            {
                auto load = [this, bufferName = String(bufferName), format, count, type, flags, staticData](ResourceHandle) -> Video::BufferPtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(bufferName), GEK_PARAMETER_TYPE(format, uint8_t), GEK_PARAMETER(count), GEK_PARAMETER_TYPE(type, uint8_t), GEK_PARAMETER(flags));
                    auto buffer = device->createBuffer(format, count, type, flags, (void *)staticData.data());
                    buffer->setName(bufferName);
                    return buffer;
                };

                std::size_t hash = std::hash<String>()(bufferName);
                return resourceManager.getHandle(hash, load);
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
                auto load = [this, textureName = String(textureName), flags](ResourceHandle) -> Video::TexturePtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(textureName), GEK_PARAMETER(flags));
                    return loadTextureData(textureName, flags);
                };

                std::size_t hash = std::hash<String>()(textureName);
                return resourceManager.getHandle(hash, load);
            }

            ResourceHandle createTexture(const wchar_t *pattern, const wchar_t *parameters)
            {
                GEK_REQUIRE(pattern);
                GEK_REQUIRE(parameters);

                auto load = [this, pattern = String(pattern), parameters = String(parameters)](ResourceHandle) -> Video::TexturePtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(pattern), GEK_PARAMETER(parameters));
                    return createTextureData(pattern, parameters);
                };

                std::size_t hash = std::hash_combine<String, String>(pattern, parameters);
                return resourceManager.getHandle(hash, load);
            }

            ProgramHandle loadComputeProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude, const std::unordered_map<StringUTF8, StringUTF8> &definesMap)
            {
                GEK_REQUIRE(fileName);
                GEK_REQUIRE(entryFunction);

                auto load = [this, fileName = String(fileName), entryFunction = StringUTF8(entryFunction), onInclude = move(onInclude), definesMap](ProgramHandle) -> Video::ObjectPtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName), GEK_PARAMETER(entryFunction));
                    auto program = device->loadComputeProgram(fileName, entryFunction, onInclude, definesMap);
                    program->setName(String(L"%v:%v", fileName, entryFunction));
                    return program;
                };

                return programManager.getUniqueHandle(load);
            }

            ProgramHandle loadPixelProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude, const std::unordered_map<StringUTF8, StringUTF8> &definesMap)
            {
                GEK_REQUIRE(fileName);
                GEK_REQUIRE(entryFunction);

                auto load = [this, fileName = String(fileName), entryFunction = StringUTF8(entryFunction), onInclude = move(onInclude), definesMap](ProgramHandle) -> Video::ObjectPtr
                {
                    GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName), GEK_PARAMETER(entryFunction));
                    auto program = device->loadPixelProgram(fileName, entryFunction, onInclude, definesMap);
                    program->setName(String(L"%v:%v", fileName, entryFunction));
                    return program;
                };

                return programManager.getUniqueHandle(load);
            }

            void mapBuffer(ResourceHandle bufferHandle, void **data)
            {
                GEK_REQUIRE(data);

                auto buffer = resourceManager.getResource(bufferHandle);
                GEK_CHECK_CONDITION(buffer == nullptr, Resources::ResourceNotLoaded, "Buffer not loaded: %v", bufferHandle.identifier);
                device->mapBuffer(dynamic_cast<Video::Buffer *>(buffer), data);
            }

            void unmapBuffer(ResourceHandle bufferHandle)
            {
                auto buffer = resourceManager.getResource(bufferHandle);
                GEK_CHECK_CONDITION(buffer == nullptr, Resources::ResourceNotLoaded, "Buffer not loaded: %v", bufferHandle.identifier);
                device->unmapBuffer(dynamic_cast<Video::Buffer *>(buffer));
            }

            void generateMipMaps(Video::Device::Context *deviceContext, ResourceHandle resourceHandle)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                GEK_CHECK_CONDITION(resource == nullptr, Resources::ResourceNotLoaded, "Resource not loaded: %v", resourceHandle.identifier);
                deviceContext->generateMipMaps(dynamic_cast<Video::Texture *>(resource));
            }

            void copyResource(ResourceHandle sourceHandle, ResourceHandle destinationHandle)
            {
                auto source = resourceManager.getResource(sourceHandle);
                auto destination = resourceManager.getResource(destinationHandle);
                GEK_CHECK_CONDITION(source == nullptr, Resources::ResourceNotLoaded, "Source of copy resource not loaded: %v", sourceHandle.identifier);
                GEK_CHECK_CONDITION(destination == nullptr, Resources::ResourceNotLoaded, "Destination of copy resource not loaded: %v", destinationHandle.identifier);
                device->copyResource(source, destination);
            }

            void setRenderState(Video::Device::Context *deviceContext, RenderStateHandle renderStateHandle)
            {
                GEK_REQUIRE(deviceContext);

                auto renderState = renderStateManager.getResource(renderStateHandle);
                GEK_CHECK_CONDITION(renderState == nullptr, Resources::ResourceNotLoaded, "Render state not loaded: %v", renderStateHandle.identifier);
                deviceContext->setRenderState(renderState);
            }

            void setDepthState(Video::Device::Context *deviceContext, DepthStateHandle depthStateHandle, uint32_t stencilReference)
            {
                GEK_REQUIRE(deviceContext);

                auto depthState = depthStateManager.getResource(depthStateHandle);
                GEK_CHECK_CONDITION(depthState == nullptr, Resources::ResourceNotLoaded, "Depth state not loaded: %v", depthStateHandle.identifier);
                deviceContext->setDepthState(depthState, stencilReference);
            }

            void setBlendState(Video::Device::Context *deviceContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, uint32_t sampleMask)
            {
                GEK_REQUIRE(deviceContext);

                auto blendState = blendStateManager.getResource(blendStateHandle);
                GEK_CHECK_CONDITION(blendState == nullptr, Resources::ResourceNotLoaded, "Blend state not loaded: %v", blendStateHandle.identifier);
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
                GEK_CHECK_CONDITION(program == nullptr, Resources::ResourceNotLoaded, "Program not loaded: %v", programHandle.identifier);
                deviceContextPipeline->setProgram(program);
            }

            void setVertexBuffer(Video::Device::Context *deviceContext, uint32_t slot, ResourceHandle resourceHandle, uint32_t offset)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                GEK_CHECK_CONDITION(resource == nullptr, Resources::ResourceNotLoaded, "Buffer not loaded: %v", resourceHandle.identifier);
                deviceContext->setVertexBuffer(slot, dynamic_cast<Video::Buffer *>(resource), offset);
            }

            void setIndexBuffer(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, uint32_t offset)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                GEK_CHECK_CONDITION(resource == nullptr, Resources::ResourceNotLoaded, "Index buffer not loaded: %v", resourceHandle.identifier);
                deviceContext->setIndexBuffer(dynamic_cast<Video::Buffer *>(resource), offset);
            }

            void clearUnorderedAccess(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const Math::Float4 &value)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                GEK_CHECK_CONDITION(resource == nullptr, Resources::ResourceNotLoaded, "Unordered access object not loaded: %v", resourceHandle.identifier);
                deviceContext->clearUnorderedAccess(resource, value);
            }

            void clearUnorderedAccess(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const uint32_t value[4])
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                GEK_CHECK_CONDITION(resource == nullptr, Resources::ResourceNotLoaded, "Unordered access object not loaded: %v", resourceHandle.identifier);
                deviceContext->clearUnorderedAccess(resource, value);
            }

            void clearRenderTarget(Video::Device::Context *deviceContext, ResourceHandle resourceHandle, const Math::Color &color)
            {
                GEK_REQUIRE(deviceContext);

                auto resource = resourceManager.getResource(resourceHandle);
                GEK_CHECK_CONDITION(resource == nullptr, Resources::ResourceNotLoaded, "Render target not loaded: %v", resourceHandle.identifier);
                deviceContext->clearRenderTarget(dynamic_cast<Video::Target *>(resource), color);
            }

            void clearDepthStencilTarget(Video::Device::Context *deviceContext, ResourceHandle depthBufferHandle, uint32_t flags, float clearDepth, uint32_t clearStencil)
            {
                GEK_REQUIRE(deviceContext);

                auto blendState = resourceManager.getResource(depthBufferHandle);
                GEK_CHECK_CONDITION(blendState == nullptr, Resources::ResourceNotLoaded, "Depth stencil target not loaded: %v", depthBufferHandle.identifier);
                deviceContext->clearDepthStencilTarget(blendState, flags, clearDepth, clearStencil);
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
