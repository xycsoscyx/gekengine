#include "GEK\Engine\Render.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Plugin.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
#include "GEK\Components\Light.h"
#include "GEK\Components\Color.h"
#include "GEK\Context\COM.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Shapes\Sphere.h"
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <ppl.h>
#include <future>
#include <set>

namespace Gek
{
    namespace String
    {
        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(Video::Format value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << static_cast<UINT8>(value);
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(Video::BufferType value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << static_cast<UINT8>(value);
            return stream.str().c_str();
        }
    }; // namespace String

    DECLARE_INTERFACE(ResourceLoader)
    {
        STDMETHOD_(void, requestLoad)           (THIS_ LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **)> loadResource, std::function<void(IUnknown *)> setResource) PURE;
        STDMETHOD_(void, requestLoad)           (THIS_ LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **, IUnknown **)> loadResource, std::function<void(IUnknown *, IUnknown *)> setResource) PURE;
    };

    template <class HANDLE>
    class ResourceManager
    {
    public:
        struct AtomicResource
        {
            enum class State : UINT8
            {
                Empty = 0,
                Loading,
                Loaded,
            };

            std::atomic<State> state;
            CComPtr<IUnknown> data;

            AtomicResource(void)
                : state(State::Empty)
            {
            }

            AtomicResource(const AtomicResource &resource)
                : state(resource.state == State::Empty ? State::Empty : resource.state == State::Loading ? State::Loading : State::Loaded)
                , data(resource.data)
            {
            }

            void set(IUnknown *data)
            {
                if (state == State::Empty)
                {
                    state = State::Loading;
                    this->data = data;
                    state = State::Loaded;
                }
            }

            IUnknown *get(void)
            {
                return (state == State::Loaded ? data.p : nullptr);
            }
        };

        struct ReadWriteResource
        {
            AtomicResource read;
            AtomicResource write;

            ReadWriteResource(void)
            {
            }

            void set(IUnknown *read, IUnknown *write)
            {
                this->read.set(read);
                this->write.set(write);
            }
        };

    private:
        UINT64 nextIdentifier;
        concurrency::concurrent_unordered_set<std::size_t> requestedLoadSet;
        concurrency::concurrent_unordered_map<std::size_t, HANDLE> resourceHandleMap;
        concurrency::concurrent_unordered_map<HANDLE, AtomicResource> localResourceMap;
        concurrency::concurrent_unordered_map<HANDLE, ReadWriteResource> readWriteResourceMap;
        concurrency::concurrent_unordered_map<HANDLE, AtomicResource> globalResourceMap;

    public:
        ResourceManager(void)
            : nextIdentifier(0)
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

        HANDLE getGlobalHandle(ResourceLoader *resourceLoader, std::function<HRESULT(LPCVOID, IUnknown **)> loadResource)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle, IUnknown *data) -> void
            {
                auto &resource = globalResourceMap[handle];
                resource.set(data);
            }, handle, std::placeholders::_1));
            return handle;
        }

        HANDLE getUniqueHandle(ResourceLoader *resourceLoader, std::function<HRESULT(LPCVOID, IUnknown **)> loadResource)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle, IUnknown *data) -> void
            {
                auto &resource = localResourceMap[handle];
                resource.set(data);
            }, handle, std::placeholders::_1));
            return handle;
        }

        HANDLE getUniqueReadWriteHandle(ResourceLoader *resourceLoader, std::function<HRESULT(LPCVOID, IUnknown **, IUnknown **)> loadResource)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle, IUnknown *read, IUnknown *write) -> void
            {
                auto &resource = readWriteResourceMap[handle];
                resource.set(read, write);
            }, handle, std::placeholders::_1));
            return handle;
        }

        HANDLE getHandle(std::size_t hash, ResourceLoader *resourceLoader, std::function<HRESULT(LPCVOID, IUnknown **)> loadResource)
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
                resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle, IUnknown *data) -> void
                {
                    auto &resource = localResourceMap[handle];
                    resource.set(data);
                }, handle, std::placeholders::_1));
            }

            return handle;
        }

        HANDLE getReadWriteHandle(std::size_t hash, ResourceLoader *resourceLoader, std::function<HRESULT(LPCVOID, IUnknown **, IUnknown **)> loadResource)
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
                resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle, IUnknown *read, IUnknown *write) -> void
                {
                    auto &resource = readWriteResourceMap[handle];
                    resource.set(read, write);
                }, handle, std::placeholders::_1, std::placeholders::_2));
            }

            return handle;
        }

        bool getHandle(std::size_t hash, HANDLE **handle)
        {
            if (requestedLoadSet.count(hash) > 0)
            {
                auto resourceIterator = resourceHandleMap.find(hash);
                if (resourceIterator != resourceHandleMap.end())
                {
                    (*handle) = &resourceIterator->second;
                    return true;
                }
            }

            return false;
        }

        IUnknown *getResource(HANDLE handle, bool writable = false)
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

        template <typename TYPE>
        TYPE *getResource(HANDLE handle, bool writable = false)
        {
            return dynamic_cast<TYPE *>(getResource(handle, writable));
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

    class ResourcesImplementation : public ContextUserMixin
        , public Resources
        , public ResourceLoader
    {
    private:
        IUnknown *initializerContext;
        VideoSystem *video;

        ResourceManager<ProgramHandle> programManager;
        ResourceManager<PluginHandle> pluginManager;
        ResourceManager<MaterialHandle> materialManager;
        ResourceManager<ShaderHandle> shaderManager;
        ResourceManager<ResourceHandle> resourceManager;
        ResourceManager<RenderStatesHandle> renderStateManager;
        ResourceManager<DepthStatesHandle> depthStateManager;
        ResourceManager<BlendStatesHandle> blendStateManager;

        concurrency::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

        std::future<void> loadResourceRunning;
        concurrency::concurrent_queue<std::function<void(void)>> loadResourceQueue;

    public:
        ResourcesImplementation(void)
            : initializerContext(nullptr)
            , video(nullptr)
        {
        }

        ~ResourcesImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ResourcesImplementation)
            INTERFACE_LIST_ENTRY_COM(PluginResources)
            INTERFACE_LIST_ENTRY_COM(Resources)
        END_INTERFACE_LIST_USER

        void requestLoad(std::function<void(void)> load)
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

        STDMETHODIMP_(void) requestLoad(LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **)> loadResource, std::function<void(IUnknown *)> setResource)
        {
            requestLoad(std::bind([](LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **)> loadResource, std::function<void(IUnknown *)> setResource) -> void
            {
                CComPtr<IUnknown> resource;
                if (SUCCEEDED(loadResource(handle, &resource)) && resource)
                {
                    setResource(resource);
                }
            }, handle, loadResource, setResource));
        }

        STDMETHODIMP_(void) requestLoad(LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **, IUnknown **)> loadResource, std::function<void(IUnknown *, IUnknown *)> setResource)
        {
            requestLoad(std::bind([](LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **, IUnknown **)> loadResource, std::function<void(IUnknown *, IUnknown *)> setResource) -> void
            {
                CComPtr<IUnknown> read, write;
                if (SUCCEEDED(loadResource(handle, &read, &write)) && read && write)
                {
                    setResource(read, write);
                }
            }, handle, loadResource, setResource));
        }

        // Resources
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            GEK_REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<VideoSystem> video(initializerContext);
            if (video)
            {
                this->video = video;
                this->initializerContext = initializerContext;
                resultValue = S_OK;
            }

            return resultValue;
        }

        STDMETHODIMP_(void) clearLocal(void)
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

        STDMETHODIMP_(ShaderHandle) getShader(MaterialHandle material)
        {
            auto shader = materialShaderMap.find(material);
            if (shader != materialShaderMap.end())
            {
                return (*shader).second;
            }

            return ShaderHandle();
        }

        STDMETHODIMP_(LPVOID) getResourceHandle(const std::type_index &type, LPCWSTR name)
        {
            if (type == typeid(ResourceHandle))
            {
                std::size_t hash = std::hash<LPCWSTR>()(name);

                ResourceHandle *handle = nullptr;
                if (resourceManager.getHandle(hash, &handle))
                {
                    return static_cast<LPVOID>(handle);
                }
            }

            return nullptr;
        }

        STDMETHODIMP_(IUnknown *) getResource(const std::type_index &type, LPCVOID handle)
        {
            if (type == typeid(PluginHandle))
            {
                return pluginManager.getResource(*static_cast<const PluginHandle *>(handle));
            }
            else if (type == typeid(ShaderHandle))
            {
                return shaderManager.getResource(*static_cast<const ShaderHandle *>(handle));
            }
            else if (type == typeid(MaterialHandle))
            {
                return materialManager.getResource(*static_cast<const MaterialHandle *>(handle));
            }
            else if (type == typeid(ResourceHandle))
            {
                return resourceManager.getResource(*static_cast<const ResourceHandle *>(handle));
            }

            return nullptr;
        };

        STDMETHODIMP_(PluginHandle) loadPlugin(LPCWSTR fileName)
        {
            return pluginManager.getGlobalHandle(this, std::bind([this](LPCVOID handle, IUnknown **returnObject, const CStringW &fileName) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                CComPtr<Plugin> plugin;
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(PluginRegistration, &plugin));
                if (plugin)
                {
                    resultValue = plugin->initialize(initializerContext, fileName);
                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = plugin->QueryInterface(returnObject);
                    }
                }

                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, fileName));
        }

        STDMETHODIMP_(MaterialHandle) loadMaterial(LPCWSTR fileName)
        {
            std::size_t hash = std::hash<CStringW>()(CStringW(fileName).MakeReverse());
            return materialManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, const CStringW &fileName) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                CComPtr<Material> material;
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(MaterialRegistration, &material));
                if (material)
                {
                    resultValue = material->initialize(initializerContext, fileName);
                    if (SUCCEEDED(resultValue))
                    {
                        materialShaderMap[*reinterpret_cast<const MaterialHandle *>(handle)] = material->getShader();
                        resultValue = material->QueryInterface(returnObject);
                    }
                }

                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, fileName));
        }

        STDMETHODIMP_(ShaderHandle) loadShader(LPCWSTR fileName)
        {
            std::size_t hash = std::hash<CStringW>()(CStringW(fileName).MakeReverse());
            return shaderManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, const CStringW &fileName) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                CComPtr<Shader> shader;
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(ShaderRegistration, &shader));
                if (shader)
                {
                    resultValue = shader->initialize(initializerContext, fileName);
                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = shader->QueryInterface(returnObject);
                    }
                }

                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, fileName));
        }

        STDMETHODIMP_(void) loadResourceList(ShaderHandle shaderHandle, LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::list<ResourceHandle> &resourceList)
        {
            Shader *shader = shaderManager.getResource<Shader>(shaderHandle);
            if (shader)
            {
                shader->loadResourceList(materialName, resourceMap, resourceList);
            }
        }

        STDMETHODIMP_(RenderStatesHandle) createRenderStates(const Video::RenderStates &renderStates)
        {
            std::size_t hash = std::hash_combine(static_cast<UINT8>(renderStates.fillMode),
                static_cast<UINT8>(renderStates.cullMode),
                renderStates.frontCounterClockwise,
                renderStates.depthBias,
                renderStates.depthBiasClamp,
                renderStates.slopeScaledDepthBias,
                renderStates.depthClipEnable,
                renderStates.scissorEnable,
                renderStates.multisampleEnable,
                renderStates.antialiasedLineEnable);
            return renderStateManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, Video::RenderStates renderStates) -> HRESULT
            {
                HRESULT resultValue = video->createRenderStates(returnObject, renderStates);
                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, renderStates));
        }

        STDMETHODIMP_(DepthStatesHandle) createDepthStates(const Video::DepthStates &depthStates)
        {
            std::size_t hash = std::hash_combine(depthStates.enable,
                static_cast<UINT8>(depthStates.writeMask),
                static_cast<UINT8>(depthStates.comparisonFunction),
                depthStates.stencilEnable,
                depthStates.stencilReadMask,
                depthStates.stencilWriteMask,
                static_cast<UINT8>(depthStates.stencilFrontStates.failOperation),
                static_cast<UINT8>(depthStates.stencilFrontStates.depthFailOperation),
                static_cast<UINT8>(depthStates.stencilFrontStates.passOperation),
                static_cast<UINT8>(depthStates.stencilFrontStates.comparisonFunction),
                static_cast<UINT8>(depthStates.stencilBackStates.failOperation),
                static_cast<UINT8>(depthStates.stencilBackStates.depthFailOperation),
                static_cast<UINT8>(depthStates.stencilBackStates.passOperation),
                static_cast<UINT8>(depthStates.stencilBackStates.comparisonFunction));
            return depthStateManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, Video::DepthStates depthStates) -> HRESULT
            {
                HRESULT resultValue = video->createDepthStates(returnObject, depthStates);
                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, depthStates));
        }

        STDMETHODIMP_(BlendStatesHandle) createBlendStates(const Video::UnifiedBlendStates &blendStates)
        {
            std::size_t hash = std::hash_combine(blendStates.enable,
                static_cast<UINT8>(blendStates.colorSource),
                static_cast<UINT8>(blendStates.colorDestination),
                static_cast<UINT8>(blendStates.colorOperation),
                static_cast<UINT8>(blendStates.alphaSource),
                static_cast<UINT8>(blendStates.alphaDestination),
                static_cast<UINT8>(blendStates.alphaOperation),
                blendStates.writeMask);
            return blendStateManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, Video::UnifiedBlendStates blendStates) -> HRESULT
            {
                HRESULT resultValue = video->createBlendStates(returnObject, blendStates);
                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, blendStates));
        }

        STDMETHODIMP_(BlendStatesHandle) createBlendStates(const Video::IndependentBlendStates &blendStates)
        {
            std::size_t hash = 0;
            for (UINT32 renderTarget = 0; renderTarget < 8; ++renderTarget)
            {
                if (blendStates.targetStates[renderTarget].enable)
                {
                    std::hash_combine(hash, std::hash_combine(renderTarget,
                        static_cast<UINT8>(blendStates.targetStates[renderTarget].colorSource),
                        static_cast<UINT8>(blendStates.targetStates[renderTarget].colorDestination),
                        static_cast<UINT8>(blendStates.targetStates[renderTarget].colorOperation),
                        static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaSource),
                        static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaDestination),
                        static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaOperation),
                        blendStates.targetStates[renderTarget].writeMask));
                }
            }

            return blendStateManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, Video::IndependentBlendStates blendStates) -> HRESULT
            {
                HRESULT resultValue = video->createBlendStates(returnObject, blendStates);
                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, blendStates));
        }

        STDMETHODIMP_(ResourceHandle) createTexture(LPCWSTR name, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps)
        {
            if (name)
            {
                std::size_t hash = std::hash<LPCWSTR>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **readObject, IUnknown **writeObject, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps) -> HRESULT
                    {
                        CComPtr<VideoTexture> read, write;
                        HRESULT resultValue = video->createTexture(&read, format, width, height, depth, flags, mipmaps);
                        resultValue |= video->createTexture(&write, format, width, height, depth, flags, mipmaps);
                        if (SUCCEEDED(resultValue) && read && write)
                        {
                            resultValue = read->QueryInterface(readObject);
                            resultValue |= write->QueryInterface(writeObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, format, width, height, depth, flags, mipmaps));
                }
                else
                {
                    return resourceManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps) -> HRESULT
                    {
                        CComPtr<VideoTexture> texture;
                        HRESULT resultValue = video->createTexture(&texture, format, width, height, depth, flags, mipmaps);
                        if (SUCCEEDED(resultValue) && texture)
                        {
                            resultValue = texture->QueryInterface(returnObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, std::placeholders::_2, format, width, height, depth, flags, mipmaps));
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(this, std::bind([this](LPCVOID handle, IUnknown **returnObject, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps) -> HRESULT
                {
                    CComPtr<VideoTexture> texture;
                    HRESULT resultValue = video->createTexture(&texture, format, width, height, depth, flags, mipmaps);
                    if (SUCCEEDED(resultValue) && texture)
                    {
                        resultValue = texture->QueryInterface(returnObject);
                    }

                    return resultValue;
                }, std::placeholders::_1, std::placeholders::_2, format, width, height, depth, flags, mipmaps));
            }
        }

        STDMETHODIMP_(ResourceHandle) createBuffer(LPCWSTR name, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData)
        {
            if (name)
            {
                std::size_t hash = std::hash<LPCWSTR>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **readObject, IUnknown **writeObject, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> HRESULT
                    {
                        CComPtr<VideoBuffer> read, write;
                        HRESULT resultValue = video->createBuffer(&read, stride, count, type, flags, staticData);
                        resultValue |= video->createBuffer(&write, stride, count, type, flags, staticData);
                        if (SUCCEEDED(resultValue) && read && write)
                        {
                            resultValue = read->QueryInterface(readObject);
                            resultValue |= write->QueryInterface(writeObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, stride, count, type, flags, staticData));
                }
                else
                {
                    return resourceManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> HRESULT
                    {
                        CComPtr<VideoBuffer> buffer;
                        HRESULT resultValue = video->createBuffer(&buffer, stride, count, type, flags, staticData);
                        if (SUCCEEDED(resultValue) && buffer)
                        {
                            resultValue = buffer->QueryInterface(returnObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, std::placeholders::_2, stride, count, type, flags, staticData));
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(this, std::bind([this](LPCVOID handle, IUnknown **returnObject, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> HRESULT
                {
                    CComPtr<VideoBuffer> buffer;
                    HRESULT resultValue = video->createBuffer(&buffer, stride, count, type, flags, staticData);
                    if (SUCCEEDED(resultValue) && buffer)
                    {
                        resultValue = buffer->QueryInterface(returnObject);
                    }

                    return resultValue;
                }, std::placeholders::_1, std::placeholders::_2, stride, count, type, flags, staticData));
            }
        }

        STDMETHODIMP_(ResourceHandle) createBuffer(LPCWSTR name, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData)
        {
            if (name)
            {
                std::size_t hash = std::hash<LPCWSTR>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **readObject, IUnknown **writeObject, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> HRESULT
                    {
                        CComPtr<VideoBuffer> read, write;
                        HRESULT resultValue = video->createBuffer(&read, format, count, type, flags, staticData);
                        resultValue |= video->createBuffer(&write, format, count, type, flags, staticData);
                        if (SUCCEEDED(resultValue) && read && write)
                        {
                            resultValue = read->QueryInterface(readObject);
                            resultValue |= write->QueryInterface(writeObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, format, count, type, flags, staticData));
                }
                else
                {
                    return resourceManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> HRESULT
                    {
                        CComPtr<VideoBuffer> buffer;
                        HRESULT resultValue = video->createBuffer(&buffer, format, count, type, flags, staticData);
                        if (SUCCEEDED(resultValue) && buffer)
                        {
                            resultValue = buffer->QueryInterface(returnObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, std::placeholders::_2, format, count, type, flags, staticData));
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(this, std::bind([this](LPCVOID handle, IUnknown **returnObject, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> HRESULT
                {
                    CComPtr<VideoBuffer> buffer;
                    HRESULT resultValue = video->createBuffer(&buffer, format, count, type, flags, staticData);
                    if (SUCCEEDED(resultValue) && buffer)
                    {
                        resultValue = buffer->QueryInterface(returnObject);
                    }

                    return resultValue;
                }, std::placeholders::_1, std::placeholders::_2, format, count, type, flags, staticData));
            }
        }

        HRESULT createTexture(VideoTexture **returnObject, CStringW parameters, UINT32 flags)
        {
            GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            CComPtr<VideoTexture> texture;

            int position = 0;
            CStringW type(parameters.Tokenize(L":", position));
            CStringW value(parameters.Tokenize(L":", position));
            if (type.CompareNoCase(L"color") == 0)
            {
                UINT8 colorData[4] = { 0, 0, 0, 0 };
                UINT32 colorPitch = 0;

                float color1;
                Math::Float2 color2;
                Math::Float3 color3;
                Math::Float4 color4;
                if (Evaluator::get(value, color1))
                {
                    resultValue = video->createTexture(&texture, Video::Format::Byte, 1, 1, 1, Video::TextureFlags::Resource);
                    colorData[0] = UINT8(color1 * 255.0f);
                    colorPitch = 1;
                }
                else if (Evaluator::get(value, color2))
                {
                    resultValue = video->createTexture(&texture, Video::Format::Byte2, 1, 1, 1, Video::TextureFlags::Resource);
                    colorData[0] = UINT8(color2.x * 255.0f);
                    colorData[1] = UINT8(color2.y * 255.0f);
                    colorPitch = 2;
                }
                else if (Evaluator::get(value, color3))
                {
                    resultValue = video->createTexture(&texture, Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                    colorData[0] = UINT8(color3.x * 255.0f);
                    colorData[1] = UINT8(color3.y * 255.0f);
                    colorData[2] = UINT8(color3.z * 255.0f);
                    colorData[3] = 255;
                    colorPitch = 4;
                }
                else if (Evaluator::get(value, color4))
                {
                    resultValue = video->createTexture(&texture, Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                    colorData[0] = UINT8(color4.x * 255.0f);
                    colorData[1] = UINT8(color4.y * 255.0f);
                    colorData[2] = UINT8(color4.z * 255.0f);
                    colorData[3] = UINT8(color4.w * 255.0f);
                    colorPitch = 4;
                }

                if (texture && colorPitch > 0)
                {
                    video->updateTexture(texture, colorData, colorPitch);
                }
            }
            else if (type.CompareNoCase(L"normal") == 0)
            {
                resultValue = video->createTexture(&texture, Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                if (texture)
                {
                    Math::Float3 normal((Evaluator::get<Math::Float3>(value).getNormal() + 1.0f) * 0.5f);
                    UINT8 normalData[4] = 
                    {
                        UINT8(normal.x * 255.0f),
                        UINT8(normal.y * 255.0f),
                        UINT8(normal.z * 255.0f),
                        255,
                    };

                    video->updateTexture(texture, normalData, 4);
                }
            }
            else if (type.CompareNoCase(L"pattern") == 0)
            {
                if (value.CompareNoCase(L"debug") == 0)
                {
                    resultValue = video->createTexture(&texture, Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                    if (texture)
                    {
                        UINT8 data[] =
                        {
                            255, 0, 255, 255,
                        };

                        video->updateTexture(texture, data, 4);
                    }
                }
                else if (value.CompareNoCase(L"flat") == 0)
                {
                    resultValue = video->createTexture(&texture, Video::Format::Byte4, 1, 1, 1, Video::TextureFlags::Resource);
                    if (texture)
                    {
                        UINT8 normalData[4] =
                        {
                            127,
                            127,
                            255,
                            255,
                        };

                        video->updateTexture(texture, normalData, 4);
                    }
                }
            }

            if (texture)
            {
                resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
            }

            return resultValue;
        }

        HRESULT loadTexture(VideoTexture **returnObject, LPCWSTR fileName, UINT32 flags)
        {
            GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            // iterate over formats in case the texture name has no extension
            static LPCWSTR formatList[] =
            {
                L"",
                L".dds",
                L".tga",
                L".png",
                L".jpg",
                L".bmp",
            };

            CComPtr<VideoTexture> texture;
            for (auto &format : formatList)
            {
                CStringW fullFileName(FileSystem::expandPath(String::format(L"%%root%%\\data\\textures\\%s%s", fileName, format)));
                if (PathFileExists(fullFileName))
                {
                    resultValue = video->loadTexture(&texture, fullFileName, flags);
                    if (SUCCEEDED(resultValue))
                    {
                        break;
                    }
                }
            }

            if (texture)
            {
                resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
            }

            return resultValue;
        }

        STDMETHODIMP_(ResourceHandle) loadTexture(LPCWSTR fileName, LPCWSTR fallback, UINT32 flags)
        {
            std::size_t hash = std::hash<CStringW>()(CStringW(fileName).MakeReverse());
            return resourceManager.getHandle(hash, this, std::bind([this](LPCVOID handle, IUnknown **returnObject, const CStringW &fileName, const CStringW &fallback, UINT32 flags) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;
                CComPtr<VideoTexture> texture;
                if (fileName.GetAt(0) == L'*')
                {
                    resultValue = createTexture(&texture, fileName.Mid(1), flags);
                }
                else
                {
                    resultValue = loadTexture(&texture, fileName, flags);
                    if ((FAILED(resultValue) || !texture) && fallback.GetAt(0) == L'*')
                    {
                        resultValue = createTexture(&texture, fallback.Mid(1), flags);
                    }
                }

                if (texture)
                {
                    resultValue = texture->QueryInterface(returnObject);
                }

                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, fileName, fallback, flags));
        }

        STDMETHODIMP_(ProgramHandle) loadComputeProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            return programManager.getUniqueHandle(this, std::bind([this](LPCVOID handle, IUnknown **returnObject, const CStringW &fileName, const CStringA &entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> defineList) -> HRESULT
            {
                HRESULT resultValue = video->loadComputeProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, fileName, entryFunction, onInclude, defineList));
        }

        STDMETHODIMP_(ProgramHandle) loadPixelProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            return programManager.getUniqueHandle(this, std::bind([this](LPCVOID handle, IUnknown **returnObject, const CStringW &fileName, const CStringA &entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> defineList) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;
                resultValue = video->loadPixelProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                return resultValue;
            }, std::placeholders::_1, std::placeholders::_2, fileName, entryFunction, onInclude, defineList));
        }

        STDMETHODIMP mapBuffer(ResourceHandle buffer, void **data)
        {
            return video->mapBuffer(resourceManager.getResource<VideoBuffer>(buffer), data);
        }

        STDMETHODIMP_(void) unmapBuffer(ResourceHandle buffer)
        {
            video->unmapBuffer(resourceManager.getResource<VideoBuffer>(buffer));
        }

        STDMETHODIMP_(void) flip(ResourceHandle resourceHandle)
        {
            resourceManager.flipResource(resourceHandle);
        }

        STDMETHODIMP_(void) generateMipMaps(RenderContext *renderContext, ResourceHandle resourceHandle)
        {
            renderContext->getContext()->generateMipMaps(resourceManager.getResource<VideoTexture>(resourceHandle));
        }

        STDMETHODIMP_(void) copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle)
        {
            video->copyResource(resourceManager.getResource(destinationHandle), resourceManager.getResource(sourceHandle));
        }

        STDMETHODIMP_(void) setRenderStates(RenderContext *renderContext, RenderStatesHandle renderStatesHandle)
        {
            renderContext->getContext()->setRenderStates(renderStateManager.getResource<IUnknown>(renderStatesHandle));
        }

        STDMETHODIMP_(void) setDepthStates(RenderContext *renderContext, DepthStatesHandle depthStatesHandle, UINT32 stencilReference)
        {
            renderContext->getContext()->setDepthStates(depthStateManager.getResource<IUnknown>(depthStatesHandle), stencilReference);
        }

        STDMETHODIMP_(void) setBlendStates(RenderContext *renderContext, BlendStatesHandle blendStatesHandle, const Math::Color &blendFactor, UINT32 sampleMask)
        {
            renderContext->getContext()->setBlendStates(blendStateManager.getResource<IUnknown>(blendStatesHandle), blendFactor, sampleMask);
        }

        STDMETHODIMP_(void) setResource(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setResource(resourceManager.getResource<IUnknown>(resourceHandle), stage);
        }

        STDMETHODIMP_(void) setUnorderedAccess(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setUnorderedAccess(resourceManager.getResource<IUnknown>(resourceHandle, true), stage);
        }

        STDMETHODIMP_(void) setConstantBuffer(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setConstantBuffer(resourceManager.getResource<VideoBuffer>(resourceHandle), stage);
        }

        STDMETHODIMP_(void) setProgram(RenderPipeline *renderPipeline, ProgramHandle programHandle)
        {
            renderPipeline->getPipeline()->setProgram(programManager.getResource<IUnknown>(programHandle));
        }

        STDMETHODIMP_(void) setVertexBuffer(RenderContext *renderContext, UINT32 slot, ResourceHandle resourceHandle, UINT32 offset)
        {
            renderContext->getContext()->setVertexBuffer(slot, resourceManager.getResource<VideoBuffer>(resourceHandle), offset);
        }

        STDMETHODIMP_(void) setIndexBuffer(RenderContext *renderContext, ResourceHandle resourceHandle, UINT32 offset)
        {
            renderContext->getContext()->setIndexBuffer(resourceManager.getResource<VideoBuffer>(resourceHandle), offset);
        }

        STDMETHODIMP_(void) clearRenderTarget(RenderContext *renderContext, ResourceHandle resourceHandle, const Math::Color &color)
        {
            renderContext->getContext()->clearRenderTarget(resourceManager.getResource<VideoTarget>(resourceHandle, true), color);
        }

        STDMETHODIMP_(void) clearDepthStencilTarget(RenderContext *renderContext, ResourceHandle depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear)
        {
            renderContext->getContext()->clearDepthStencilTarget(resourceManager.getResource<IUnknown>(depthBuffer), flags, depthClear, stencilClear);
        }

        Video::ViewPort viewPortList[8];
        VideoTarget *renderTargetList[8];
        STDMETHODIMP_(void) setRenderTargets(RenderContext *renderContext, ResourceHandle *renderTargetHandleList, UINT32 renderTargetHandleCount, ResourceHandle *depthBuffer)
        {
            for (UINT32 renderTarget = 0; renderTarget < renderTargetHandleCount; renderTarget++)
            {
                renderTargetList[renderTarget] = resourceManager.getResource<VideoTarget>(renderTargetHandleList[renderTarget], true);
                if (renderTargetList[renderTarget])
                {
                    viewPortList[renderTarget] = renderTargetList[renderTarget]->getViewPort();
                }
            }

            renderContext->getContext()->setRenderTargets(renderTargetList, renderTargetHandleCount, (depthBuffer ? resourceManager.getResource<IUnknown>(*depthBuffer) : nullptr));
            renderContext->getContext()->setViewports(viewPortList, renderTargetHandleCount);
        }

        STDMETHODIMP_(void) setBackBuffer(RenderContext *renderContext, ResourceHandle *depthBuffer)
        {
            auto backBuffer = video->getBackBuffer();
            renderTargetList[0] = backBuffer;
            if (renderTargetList[0])
            {
                viewPortList[0] = renderTargetList[0]->getViewPort();
            }

            renderContext->getContext()->setRenderTargets(renderTargetList, 1, (depthBuffer ? resourceManager.getResource<IUnknown>(*depthBuffer) : nullptr));
            renderContext->getContext()->setViewports(viewPortList, 1);
        }
    };

    REGISTER_CLASS(ResourcesImplementation)
}; // namespace Gek
