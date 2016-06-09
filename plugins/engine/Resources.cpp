#include "GEK\Utility\String.h"
#include "GEK\Utility\ShuntingYard.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Plugin.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
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
    static ShuntingYard shuntingYard;

    std::size_t reverseHash(const std::wstring &string)
    {
        return std::hash<std::wstring>()(std::wstring(string.rbegin(), string.rend()));
    }

    GEK_INTERFACE(RequestLoader)
    {
        virtual void request(std::function<void(void)> load) = 0;
    };

    template <class HANDLE, typename TYPE>
    class ResourceManager
    {
    public:
        typedef std::shared_ptr<TYPE> TypePtr;

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
            loader->request([this, handle, requestLoad](void) -> void
            {
                requestLoad(handle, [this, handle](TypePtr data) -> void
                {
                    auto &resource = globalResourceMap[handle];
                    resource.set(data);
                });
            });

            return handle;
        }

        HANDLE getUniqueHandle(std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            loader->request([this, handle, requestLoad](void) -> void
            {
                requestLoad(handle, [this, handle](TypePtr data) -> void
                {
                    auto &resource = localResourceMap[handle];
                    resource.set(data);
                });
            });

            return handle;
        }

        HANDLE getUniqueReadWriteHandle(std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            loader->request([this, handle, requestLoad](void) -> void
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
            });

            return handle;
        }

        HANDLE getHandle(std::size_t hash, std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            if (requestedLoadSet.count(hash) > 0)
            {
                auto resourceIterator = resourceHandleMap.find(hash);
                if (resourceIterator != resourceHandleMap.end())
                {
                    handle = resourceIterator->second;
                }
            }
            else
            {
                requestedLoadSet.insert(hash);
                handle.assign(InterlockedIncrement(&nextIdentifier));
                resourceHandleMap[hash] = handle;
                loader->request([this, handle, requestLoad](void) -> void
                {
                    requestLoad(handle, [this, handle](TypePtr data) -> void
                    {
                        auto &resource = localResourceMap[handle];
                        resource.set(data);
                    });
                });
            }

            return handle;
        }

        HANDLE getReadWriteHandle(std::size_t hash, std::function<void(HANDLE, std::function<void(TypePtr)>)> requestLoad)
        {
            HANDLE handle;
            if (requestedLoadSet.count(hash) > 0)
            {
                auto resourceIterator = resourceHandleMap.find(hash);
                if (resourceIterator != resourceHandleMap.end())
                {
                    handle = resourceIterator->second;
                }
            }
            else
            {
                requestedLoadSet.insert(hash);
                handle.assign(InterlockedIncrement(&nextIdentifier));
                resourceHandleMap[hash] = handle;
                loader->request([this, handle, requestLoad](void) -> void
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
                });
            }

            return handle;
        }

        HANDLE getHandle(std::size_t hash) const
        {
            if (requestedLoadSet.count(hash) > 0)
            {
                auto resourceIterator = resourceHandleMap.find(hash);
                if (resourceIterator != resourceHandleMap.end())
                {
                    return resourceIterator->second;
                }
            }

            return HANDLE();
        }

        TYPE * const getResource(HANDLE handle, bool writable = false) const
        {
            auto globalIterator = globalResourceMap.find(handle);
            if (globalIterator != globalResourceMap.end())
            {
                return globalIterator->second.get();
            }

            auto localIterator = localResourceMap.find(handle);
            if (localIterator != localResourceMap.end())
            {
                return localIterator->second.get();
            }

            auto readWriteIterator = readWriteResourceMap.find(handle);
            if (readWriteIterator != readWriteResourceMap.end())
            {
                auto &resource = readWriteIterator->second;
                return (writable ? resource.write.get() : resource.read.get());
            }

            return nullptr;
        }

        void flipResource(HANDLE handle)
        {
            auto readWriteIterator = readWriteResourceMap.find(handle);
            if (readWriteIterator != readWriteResourceMap.end())
            {
                auto &resource = readWriteIterator->second;
                if (resource.read.state == AtomicResource::State::Loaded &&
                    resource.write.state == AtomicResource::State::Loaded)
                {
                    std::swap(resource.read.data, resource.write.data);
                }
            }
        }
    };

    class ResourcesImplementation
        : public ContextRegistration<ResourcesImplementation, EngineContext *, VideoSystem *>
        , public Resources
        , public RequestLoader
    {
    private:
        EngineContext *engine;
        VideoSystem *video;

        ResourceManager<ProgramHandle, VideoObject> programManager;
        ResourceManager<PluginHandle, Plugin> pluginManager;
        ResourceManager<MaterialHandle, Material> materialManager;
        ResourceManager<ShaderHandle, Shader> shaderManager;
        ResourceManager<ResourceHandle, VideoObject> resourceManager;
        ResourceManager<RenderStateHandle, VideoObject> renderStateManager;
        ResourceManager<DepthStateHandle, VideoObject> depthStateManager;
        ResourceManager<BlendStateHandle, VideoObject> blendStateManager;

        concurrency::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

        std::future<void> loadResourceRunning;
        concurrency::concurrent_queue<std::function<void(void)>> loadResourceQueue;

    public:
        ResourcesImplementation(Context *context, EngineContext *engine, VideoSystem *video)
            : ContextRegistration(context)
            , engine(engine)
            , video(video)
            , programManager(this)
            , pluginManager(this)
            , materialManager(this)
            , shaderManager(this)
            , resourceManager(this)
            , renderStateManager(this)
            , depthStateManager(this)
            , blendStateManager(this)
        {
        }

        // RequestLoader
        void request(std::function<void(void)> load)
        {
            load();
            return;
            loadResourceQueue.push(load);
            if (!loadResourceRunning.valid() || (loadResourceRunning.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
            {
                loadResourceRunning = std::async(std::launch::async, [this](void) -> void
                {
                    CoInitialize(nullptr);
                    std::function<void(void)> function;
                    while (loadResourceQueue.try_pop(function))
                    {
                        function();
                    };

                    CoUninitialize();
                });
            }
        }

        // Resources
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
            resourceManager.clear();
            renderStateManager.clear();
            depthStateManager.clear();
            blendStateManager.clear();
        }

        ShaderHandle getMaterialShader(MaterialHandle material) const
        {
            auto shader = materialShaderMap.find(material);
            if (shader != materialShaderMap.end())
            {
                return (*shader).second;
            }

            return ShaderHandle();
        }

        ResourceHandle getResourceHandle(const wchar_t *name) const
        {
            std::size_t hash = std::hash<String>()(name);
            return resourceManager.getHandle(hash);
        }

        Shader * const getShader(ShaderHandle handle) const
        {
            return shaderManager.getResource(handle, false);
        }

        Plugin * const getPlugin(PluginHandle handle) const
        {
            return pluginManager.getResource(handle, false);
        }

        Material * const getMaterial(MaterialHandle handle) const
        {
            return materialManager.getResource(handle, false);
        }

        PluginHandle loadPlugin(const wchar_t *fileName)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName));
            auto load = [this, fileName = String(fileName)](PluginHandle handle)->PluginPtr
            {
                return getContext()->createClass<Plugin>(L"PluginSystem", video, fileName.c_str());
            };

            auto request = [this, load](PluginHandle handle, std::function<void(PluginPtr)> set) -> void
            {
                set(load(handle));
            };

            return pluginManager.getGlobalHandle(request);
        }

        MaterialHandle loadMaterial(const wchar_t *fileName)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName));
            auto load = [this, fileName = String(fileName)](MaterialHandle handle)->MaterialPtr
            {
                return getContext()->createClass<Material>(L"MaterialSystem", (Resources *)this, fileName.c_str());
            };

            auto request = [this, load](MaterialHandle handle, std::function<void(MaterialPtr)> set) -> void
            {
                MaterialPtr material = load(handle);
                materialShaderMap[handle] = material->getShader();
                set(load(handle));
            };

            std::size_t hash = reverseHash(fileName);
            return materialManager.getHandle(hash, request);
        }

        ShaderHandle loadShader(const wchar_t *fileName)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName));
            auto load = [this, fileName = String(fileName)](ShaderHandle handle)->ShaderPtr
            {
                return getContext()->createClass<Shader>(L"ShaderSystem", video, (Resources *)this, engine->getPopulation(), fileName.c_str());
            };

            auto request = [this, load](ShaderHandle handle, std::function<void(ShaderPtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = reverseHash(fileName);
            return shaderManager.getHandle(hash, request);
        }

        void loadResourceList(ShaderHandle shaderHandle, const wchar_t *materialName, std::unordered_map<String, ResourcePtr> &resourceMap, std::list<ResourceHandle> &resourceList)
        {
            auto shader = shaderManager.getResource(shaderHandle);
            if (shader)
            {
                shader->loadResourceList(materialName, resourceMap, resourceList);
            }
        }

        RenderStateHandle createRenderState(const Video::RenderState &renderState)
        {
            GEK_TRACE_FUNCTION();
            auto load = [this, renderState](RenderStateHandle handle) -> VideoObjectPtr
            {
                return video->createRenderState(renderState);
            };

            auto request = [this, load](RenderStateHandle handle, std::function<void(VideoObjectPtr)> set) -> void
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
            return renderStateManager.getHandle(hash, request);
        }

        DepthStateHandle createDepthState(const Video::DepthState &depthState)
        {
            GEK_TRACE_FUNCTION();
            auto load = [this, depthState](DepthStateHandle handle) -> VideoObjectPtr
            {
                return video->createDepthState(depthState);
            };

            auto request = [this, load](DepthStateHandle handle, std::function<void(VideoObjectPtr)> set) -> void
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
            return depthStateManager.getHandle(hash, request);
        }

        BlendStateHandle createBlendState(const Video::UnifiedBlendState &blendState)
        {
            GEK_TRACE_FUNCTION();
            auto load = [this, blendState](BlendStateHandle handle) -> VideoObjectPtr
            {
                return video->createBlendState(blendState);
            };

            auto request = [this, load](BlendStateHandle handle, std::function<void(VideoObjectPtr)> set) -> void
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
            return blendStateManager.getHandle(hash, request);
        }

        BlendStateHandle createBlendState(const Video::IndependentBlendState &blendState)
        {
            GEK_TRACE_FUNCTION();
            auto load = [this, blendState](BlendStateHandle handle) -> VideoObjectPtr
            {
                return video->createBlendState(blendState);
            };

            auto request = [this, load](BlendStateHandle handle, std::function<void(VideoObjectPtr)> set) -> void
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

            return blendStateManager.getHandle(hash, request);
        }

        ResourceHandle createTexture(const wchar_t *name, Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t flags, uint32_t mipmaps)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(name));
            auto load = [this, format, width, height, depth, flags, mipmaps](ResourceHandle handle) -> VideoTexturePtr
            {
                return video->createTexture(format, width, height, depth, flags, mipmaps);
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoTexturePtr)> set) -> void
            {
                set(load(handle));
            };

            if (name)
            {
                std::size_t hash = std::hash<String>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, request);
                }
                else
                {
                    return resourceManager.getHandle(hash, request);
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(request);
            }
        }

        ResourceHandle createBuffer(const wchar_t *name, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const void *staticData)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(name));
            auto load = [this, stride, count, type, flags, staticData](ResourceHandle handle) -> VideoBufferPtr
            {
                return video->createBuffer(stride, count, type, flags, staticData);
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoBufferPtr)> set) -> void
            {
                set(load(handle));
            };

            if (name)
            {
                std::size_t hash = std::hash<String>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, request);
                }
                else
                {
                    return resourceManager.getHandle(hash, request);
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(request);
            }
        }

        ResourceHandle createBuffer(const wchar_t *name, Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const void *staticData)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(name));
            auto load = [this, format, count, type, flags, staticData](ResourceHandle handle) -> VideoBufferPtr
            {
                return video->createBuffer(format, count, type, flags, staticData);
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoBufferPtr)> set) -> void
            {
                set(load(handle));
            };

            if (name)
            {
                std::size_t hash = std::hash<String>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, request);
                }
                else
                {
                    return resourceManager.getHandle(hash, request);
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(request);
            }
        }

        VideoTexturePtr loadTextureData(const String &fileName, uint32_t flags)
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
                FileSystem::Path filePath(FileSystem::expandPath(String(L"$root\\data\\textures\\%v%v", fileName, format)));
                if (filePath.isFile())
                {
                    return video->loadTexture(filePath, flags);
                }
            }

            return nullptr;
        }

        VideoTexturePtr createTextureData(const String &pattern, const String &parameters)
        {
            VideoTexturePtr texture;
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

                        texture = video->createTexture(Video::Format::Byte, 1, 1, 1, Video::TextureFlags::Resource, 1, colorData);
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

                        texture = video->createTexture(Video::Format::Byte2, 1, 1, 1, Video::TextureFlags::Resource, 1, colorData);
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

                        texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource, 1, colorData);
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

                        texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource, 1, colorData);
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
                    uint8_t(normal.x * 255.0f),
                    uint8_t(normal.y * 255.0f),
                    uint8_t(normal.z * 255.0f),
                    255,
                };

                texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource, 1, normalData);
            }
            else if (pattern.compareNoCase(L"system") == 0)
            {
                if (parameters.compareNoCase(L"debug") == 0)
                {
                    uint8_t data[] =
                    {
                        255, 0, 255, 255,
                    };

                    texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource, 1, data);
                }
                else if (parameters.compareNoCase(L"flat") == 0)
                {
                    uint8_t normalData[4] =
                    {
                        127,
                        127,
                        255,
                        255,
                    };

                    texture = video->createTexture(Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource, 1, normalData);
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

            return texture;
        }

        ResourceHandle loadTexture(const wchar_t *fileName, ResourcePtr fallback, uint32_t flags)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName));
            auto load = [this, fileName = String(fileName), fallback, flags](ResourceHandle handle)->VideoTexturePtr
            {
                VideoTexturePtr texture;
                try
                {
                    texture = loadTextureData(fileName, flags);
                }
                catch (const Exception &)
                {
                    if (fallback)
                    {
                        try
                        {
                            switch (fallback->type)
                            {
                            case Resource::Type::File:
                                if (true)
                                {
                                    auto fileResource = std::dynamic_pointer_cast<FileResource>(fallback);
                                    texture = loadTextureData(fileResource->fileName, flags);
                                }

                                break;

                            case Resource::Type::Data:
                                if (true)
                                {
                                    auto dataResource = std::dynamic_pointer_cast<DataResource>(fallback);
                                    texture = createTextureData(dataResource->pattern, dataResource->parameters);
                                }

                                break;
                            };
                        }
                        catch (const Exception &)
                        {
                        };
                    }
                };

                return texture;
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoTexturePtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = reverseHash(fileName);
            return resourceManager.getHandle(hash, request);
        }

        ResourceHandle createTexture(const wchar_t *pattern, const wchar_t *parameters)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(parameters));

            auto load = [this, pattern = String(pattern), parameters = String(parameters)](ResourceHandle handle)->VideoTexturePtr
            {
                return createTextureData(pattern, parameters);
            };

            auto request = [this, load](ResourceHandle handle, std::function<void(VideoTexturePtr)> set) -> void
            {
                set(load(handle));
            };

            std::size_t hash = std::hash_combine<String, String>(pattern, parameters);
            return resourceManager.getHandle(hash, request);
        }

        ProgramHandle loadComputeProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude, const std::unordered_map<StringUTF8, StringUTF8> &defineList)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName), GEK_PARAMETER(entryFunction));
            auto load = [this, fileName = String(fileName), entryFunction = StringUTF8(entryFunction), onInclude, defineList](ProgramHandle handle)->VideoObjectPtr
            {
                return video->loadComputeProgram(fileName, entryFunction, onInclude, defineList);
            };

            auto request = [this, load](ProgramHandle handle, std::function<void(VideoObjectPtr)> set) -> void
            {
                set(load(handle));
            };

            return programManager.getUniqueHandle(request);
        }

        ProgramHandle loadPixelProgram(const wchar_t *fileName, const char *entryFunction, std::function<void(const char *, std::vector<uint8_t> &)> onInclude, const std::unordered_map<StringUTF8, StringUTF8> &defineList)
        {
            GEK_TRACE_FUNCTION(GEK_PARAMETER(fileName), GEK_PARAMETER(entryFunction));
            auto load = [this, fileName = String(fileName), entryFunction = StringUTF8(entryFunction), onInclude, defineList](ProgramHandle handle)->VideoObjectPtr
            {
                return video->loadPixelProgram(fileName, entryFunction, onInclude, defineList);
            };

            auto request = [this, load](ProgramHandle handle, std::function<void(VideoObjectPtr)> set) -> void
            {
                set(load(handle));
            };

            return programManager.getUniqueHandle(request);
        }

        void mapBuffer(ResourceHandle buffer, void **data)
        {
            video->mapBuffer(dynamic_cast<VideoBuffer *>(resourceManager.getResource(buffer)), data);
        }

        void unmapBuffer(ResourceHandle buffer)
        {
            video->unmapBuffer(dynamic_cast<VideoBuffer *>(resourceManager.getResource(buffer)));
        }

        void flip(ResourceHandle resourceHandle)
        {
            resourceManager.flipResource(resourceHandle);
        }

        void generateMipMaps(RenderContext *renderContext, ResourceHandle resourceHandle)
        {
            renderContext->getContext()->generateMipMaps(dynamic_cast<VideoTexture *>(resourceManager.getResource(resourceHandle)));
        }

        void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle)
        {
            video->copyResource(resourceManager.getResource(destinationHandle), resourceManager.getResource(sourceHandle));
        }

        void setRenderState(RenderContext *renderContext, RenderStateHandle renderStateHandle)
        {
            renderContext->getContext()->setRenderState(renderStateManager.getResource(renderStateHandle));
        }

        void setDepthState(RenderContext *renderContext, DepthStateHandle depthStateHandle, uint32_t stencilReference)
        {
            renderContext->getContext()->setDepthState(depthStateManager.getResource(depthStateHandle), stencilReference);
        }

        void setBlendState(RenderContext *renderContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, uint32_t sampleMask)
        {
            renderContext->getContext()->setBlendState(blendStateManager.getResource(blendStateHandle), blendFactor, sampleMask);
        }

        void setResource(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, uint32_t stage)
        {
            renderPipeline->getPipeline()->setResource(resourceManager.getResource(resourceHandle), stage);
        }

        void setUnorderedAccess(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, uint32_t stage)
        {
            renderPipeline->getPipeline()->setUnorderedAccess(resourceManager.getResource(resourceHandle, true), stage);
        }

        void setConstantBuffer(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, uint32_t stage)
        {
            renderPipeline->getPipeline()->setConstantBuffer(dynamic_cast<VideoBuffer *>(resourceManager.getResource(resourceHandle)), stage);
        }

        void setProgram(RenderPipeline *renderPipeline, ProgramHandle programHandle)
        {
            renderPipeline->getPipeline()->setProgram(programManager.getResource(programHandle));
        }

        void setVertexBuffer(RenderContext *renderContext, uint32_t slot, ResourceHandle resourceHandle, uint32_t offset)
        {
            renderContext->getContext()->setVertexBuffer(slot, dynamic_cast<VideoBuffer *>(resourceManager.getResource(resourceHandle)), offset);
        }

        void setIndexBuffer(RenderContext *renderContext, ResourceHandle resourceHandle, uint32_t offset)
        {
            renderContext->getContext()->setIndexBuffer(dynamic_cast<VideoBuffer *>(resourceManager.getResource(resourceHandle)), offset);
        }

        void clearRenderTarget(RenderContext *renderContext, ResourceHandle resourceHandle, const Math::Color &color)
        {
            renderContext->getContext()->clearRenderTarget(dynamic_cast<VideoTarget *>(resourceManager.getResource(resourceHandle, true)), color);
        }

        void clearDepthStencilTarget(RenderContext *renderContext, ResourceHandle depthBuffer, uint32_t flags, float depthClear, uint32_t stencilClear)
        {
            renderContext->getContext()->clearDepthStencilTarget(resourceManager.getResource(depthBuffer), flags, depthClear, stencilClear);
        }

        Video::ViewPort viewPortList[8];
        VideoTarget *renderTargetList[8];
        void setRenderTargets(RenderContext *renderContext, ResourceHandle *renderTargetHandleList, uint32_t renderTargetHandleCount, ResourceHandle *depthBuffer)
        {
            for (uint32_t renderTarget = 0; renderTarget < renderTargetHandleCount; renderTarget++)
            {
                renderTargetList[renderTarget] = dynamic_cast<VideoTarget *>(resourceManager.getResource(renderTargetHandleList[renderTarget], true));
                if (renderTargetList[renderTarget])
                {
                    viewPortList[renderTarget] = renderTargetList[renderTarget]->getViewPort();
                }
            }

            renderContext->getContext()->setRenderTargets(renderTargetList, renderTargetHandleCount, (depthBuffer ? resourceManager.getResource(*depthBuffer) : nullptr));
            renderContext->getContext()->setViewports(viewPortList, renderTargetHandleCount);
        }

        void setBackBuffer(RenderContext *renderContext, ResourceHandle *depthBuffer)
        {
            auto backBuffer = video->getBackBuffer();
            renderTargetList[0] = backBuffer;
            if (renderTargetList[0])
            {
                viewPortList[0] = renderTargetList[0]->getViewPort();
            }

            renderContext->getContext()->setRenderTargets(renderTargetList, 1, (depthBuffer ? resourceManager.getResource(*depthBuffer) : nullptr));
            renderContext->getContext()->setViewports(viewPortList, 1);
        }
    };

    GEK_REGISTER_CONTEXT_USER(ResourcesImplementation);
}; // namespace Gek
