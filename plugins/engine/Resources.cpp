#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include "GEK\Context\ContextUser.h"
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
#include "GEK\Context\ObservableMixin.h"
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

    GEK_INTERFACE(ResourceLoader)
    {
        virtual void requestLoad(LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **)> loadResource, std::function<void(IUnknown *)> setResource) = 0;
        virtual void requestLoad(LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **, IUnknown **)> loadResource, std::function<void(IUnknown *, IUnknown *)> setResource) = 0;
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
            VideoObjectPtr data;

            AtomicResource(void)
                : state(State::Empty)
            {
            }

            AtomicResource(const AtomicResource &resource)
                : state(resource.state == State::Empty ? State::Empty : resource.state == State::Loading ? State::Loading : State::Loaded)
                , data(resource.data)
            {
            }

            void set(VideoObject *data)
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

            void set(VideoObject *read, VideoObject *write)
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

        HANDLE getGlobalHandle(ResourceLoader *resourceLoader, std::function<VideoObjectPtr(LPCVOID)> loadResource)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle) -> void
            {
                auto &resource = globalResourceMap[handle];
                resource.set(data);
            }, handle, std::placeholders::_1));
            return handle;
        }

        HANDLE getUniqueHandle(ResourceLoader *resourceLoader, std::function<VideoObjectPtr(LPCVOID)> loadResource)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle) -> void
            {
                auto &resource = localResourceMap[handle];
                resource.set(data);
            }, handle, std::placeholders::_1));
            return handle;
        }

        HANDLE getUniqueReadWriteHandle(ResourceLoader *resourceLoader, std::function<VideoObjectPtr(LPCVOID)> loadResource)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle) -> void
            {
                auto &resource = readWriteResourceMap[handle];
                resource.set(read, write);
            }, handle, std::placeholders::_1));
            return handle;
        }

        HANDLE getHandle(std::size_t hash, ResourceLoader *resourceLoader, std::function<VideoObjectPtr(LPCVOID)> loadResource)
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
                resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle) -> void
                {
                    auto &resource = localResourceMap[handle];
                    resource.set(data);
                }, handle, std::placeholders::_1));
            }

            return handle;
        }

        HANDLE getReadWriteHandle(std::size_t hash, ResourceLoader *resourceLoader, std::function<VideoObjectPtr(LPCVOID)> loadResource)
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
                resourceLoader->requestLoad(&handle, loadResource, std::bind([this](HANDLE handle) -> void
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

        VideoObject *getResource(HANDLE handle, bool writable = false)
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
        VideoObject *getResource(HANDLE handle, bool writable = false)
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

    class ResourcesImplementation 
        : public ContextRegistration<ResourcesImplementation>
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
        ResourceManager<RenderStateHandle> renderStateManager;
        ResourceManager<DepthStateHandle> depthStateManager;
        ResourceManager<BlendStateHandle> blendStateManager;

        concurrency::concurrent_unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

        std::future<void> loadResourceRunning;
        concurrency::concurrent_queue<std::function<void(void)>> loadResourceQueue;

    public:
        ResourcesImplementation(Context *context, VideoSystem *video)
            : ContextRegistration(context)
            , video(video)
        {
        }

        void requestLoad(std::function<void(void)> load)
        {
            static UINT32 count = 0;
            OutputDebugString(String::format(L"request: %\r\n", ++count));

            loadResourceQueue.push(load);
            if (!loadResourceRunning.valid() || (loadResourceRunning.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
            {
                loadResourceRunning = std::async(std::launch::async, [this](void) -> void
                {
                    CoInitialize(nullptr);
                    std::function<void(void)> function;
                    while (loadResourceQueue.try_pop(function))
                    {
                        OutputDebugString(String::format(L"load: %\r\n", --count));
                        function();
                    };

                    CoUninitialize();
                });
            }
        }

        void requestLoad(LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **)> loadResource, std::function<void(IUnknown *)> setResource)
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

        void requestLoad(LPCVOID handle, std::function<HRESULT(LPCVOID, IUnknown **, IUnknown **)> loadResource, std::function<void(IUnknown *, IUnknown *)> setResource)
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

        ShaderHandle getShader(MaterialHandle material)
        {
            auto shader = materialShaderMap.find(material);
            if (shader != materialShaderMap.end())
            {
                return (*shader).second;
            }

            return ShaderHandle();
        }

        void *getResourceHandle(const std::type_index &type, const wchar_t *name)
        {
            if (type == typeid(ResourceHandle))
            {
                std::size_t hash = std::hash<const wchar_t *>()(name);

                ResourceHandle *handle = nullptr;
                if (resourceManager.getHandle(hash, &handle))
                {
                    return static_cast<LPVOID>(handle);
                }
            }

            return nullptr;
        }

        VideoObject *getResource(const std::type_index &type, LPCVOID handle)
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

        PluginHandle loadPlugin(const wchar_t *fileName)
        {
            return pluginManager.getGlobalHandle(this, std::bind([this](LPCVOID handle, const wstring &fileName) -> VideoObjectPtr
            {
                return getContext()->createClass<Plugin>(L"PluginSystem");
            }, std::placeholders::_1, fileName));
        }

        MaterialHandle loadMaterial(const wchar_t *fileName)
        {
            std::size_t hash = std::hash<wstring>()(wstring(fileName).MakeReverse());
            return materialManager.getHandle(hash, this, std::bind([this](LPCVOID handle, const wstring &fileName) -> VideoObjectPtr
            {
                return getContext()->createClass<Material>(L"MaterialSystem");
            }, std::placeholders::_1, fileName));
        }

        ShaderHandle loadShader(const wchar_t *fileName)
        {
            std::size_t hash = std::hash<wstring>()(wstring(fileName).MakeReverse());
            return shaderManager.getHandle(hash, this, std::bind([this](LPCVOID handle, const wstring &fileName) -> VideoObjectPtr
            {
                return getContext()->createClass<Shader>(L"ShaderSystem");
            }, std::placeholders::_1, fileName));
        }

        void loadResourceList(ShaderHandle shaderHandle, const wchar_t *materialName, std::unordered_map<wstring, wstring> &resourceMap, std::list<ResourceHandle> &resourceList)
        {
            Shader *shader = shaderManager.getResource<Shader>(shaderHandle);
            if (shader)
            {
                shader->loadResourceList(materialName, resourceMap, resourceList);
            }
        }

        RenderStateHandle createRenderState(const Video::RenderState &renderState)
        {
            std::size_t hash = std::hash_combine(static_cast<UINT8>(renderState.fillMode),
                static_cast<UINT8>(renderState.cullMode),
                renderState.frontCounterClockwise,
                renderState.depthBias,
                renderState.depthBiasClamp,
                renderState.slopeScaledDepthBias,
                renderState.depthClipEnable,
                renderState.scissorEnable,
                renderState.multisampleEnable,
                renderState.antialiasedLineEnable);
            return renderStateManager.getHandle(hash, this, std::bind([this](LPCVOID handle, Video::RenderState renderState) -> VideoObjectPtr
            {
                HRESULT resultValue = video->createRenderState(returnObject, renderState);
                return resultValue;
            }, std::placeholders::_1, renderState));
        }

        DepthStateHandle createDepthState(const Video::DepthState &depthState)
        {
            std::size_t hash = std::hash_combine(depthState.enable,
                static_cast<UINT8>(depthState.writeMask),
                static_cast<UINT8>(depthState.comparisonFunction),
                depthState.stencilEnable,
                depthState.stencilReadMask,
                depthState.stencilWriteMask,
                static_cast<UINT8>(depthState.stencilFrontState.failOperation),
                static_cast<UINT8>(depthState.stencilFrontState.depthFailOperation),
                static_cast<UINT8>(depthState.stencilFrontState.passOperation),
                static_cast<UINT8>(depthState.stencilFrontState.comparisonFunction),
                static_cast<UINT8>(depthState.stencilBackState.failOperation),
                static_cast<UINT8>(depthState.stencilBackState.depthFailOperation),
                static_cast<UINT8>(depthState.stencilBackState.passOperation),
                static_cast<UINT8>(depthState.stencilBackState.comparisonFunction));
            return depthStateManager.getHandle(hash, this, std::bind([this](LPCVOID handle, Video::DepthState depthState) -> VideoObjectPtr
            {
                HRESULT resultValue = video->createDepthState(returnObject, depthState);
                return resultValue;
            }, std::placeholders::_1, depthState));
        }

        BlendStateHandle createBlendState(const Video::UnifiedBlendState &blendState)
        {
            std::size_t hash = std::hash_combine(blendState.enable,
                static_cast<UINT8>(blendState.colorSource),
                static_cast<UINT8>(blendState.colorDestination),
                static_cast<UINT8>(blendState.colorOperation),
                static_cast<UINT8>(blendState.alphaSource),
                static_cast<UINT8>(blendState.alphaDestination),
                static_cast<UINT8>(blendState.alphaOperation),
                blendState.writeMask);
            return blendStateManager.getHandle(hash, this, std::bind([this](LPCVOID handle, Video::UnifiedBlendState blendState) -> VideoObjectPtr
            {
                HRESULT resultValue = video->createBlendState(returnObject, blendState);
                return resultValue;
            }, std::placeholders::_1, blendState));
        }

        BlendStateHandle createBlendState(const Video::IndependentBlendState &blendState)
        {
            std::size_t hash = 0;
            for (UINT32 renderTarget = 0; renderTarget < 8; ++renderTarget)
            {
                if (blendState.targetStates[renderTarget].enable)
                {
                    std::hash_combine(hash, std::hash_combine(renderTarget,
                        static_cast<UINT8>(blendState.targetStates[renderTarget].colorSource),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].colorDestination),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].colorOperation),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].alphaSource),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].alphaDestination),
                        static_cast<UINT8>(blendState.targetStates[renderTarget].alphaOperation),
                        blendState.targetStates[renderTarget].writeMask));
                }
            }

            return blendStateManager.getHandle(hash, this, std::bind([this](LPCVOID handle, Video::IndependentBlendState blendState) -> VideoObjectPtr
            {
                HRESULT resultValue = video->createBlendState(returnObject, blendState);
                return resultValue;
            }, std::placeholders::_1, blendState));
        }

        ResourceHandle createTexture(const wchar_t *name, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps)
        {
            if (name)
            {
                std::size_t hash = std::hash<const wchar_t *>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, this, std::bind([this](LPCVOID handle, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps) -> VideoObjectPtr
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
                    }, std::placeholders::_1, format, width, height, depth, flags, mipmaps));
                }
                else
                {
                    return resourceManager.getHandle(hash, this, std::bind([this](LPCVOID handle, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps) -> VideoObjectPtr
                    {
                        CComPtr<VideoTexture> texture;
                        HRESULT resultValue = video->createTexture(&texture, format, width, height, depth, flags, mipmaps);
                        if (SUCCEEDED(resultValue) && texture)
                        {
                            resultValue = texture->QueryInterface(returnObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, format, width, height, depth, flags, mipmaps));
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(this, std::bind([this](LPCVOID handle, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags, UINT32 mipmaps) -> VideoObjectPtr
                {
                    CComPtr<VideoTexture> texture;
                    HRESULT resultValue = video->createTexture(&texture, format, width, height, depth, flags, mipmaps);
                    if (SUCCEEDED(resultValue) && texture)
                    {
                        resultValue = texture->QueryInterface(returnObject);
                    }

                    return resultValue;
                }, std::placeholders::_1, format, width, height, depth, flags, mipmaps));
            }
        }

        ResourceHandle createBuffer(const wchar_t *name, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData)
        {
            if (name)
            {
                std::size_t hash = std::hash<const wchar_t *>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, this, std::bind([this](LPCVOID handle, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> VideoObjectPtr
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
                    }, std::placeholders::_1, stride, count, type, flags, staticData));
                }
                else
                {
                    return resourceManager.getHandle(hash, this, std::bind([this](LPCVOID handle, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> VideoObjectPtr
                    {
                        CComPtr<VideoBuffer> buffer;
                        HRESULT resultValue = video->createBuffer(&buffer, stride, count, type, flags, staticData);
                        if (SUCCEEDED(resultValue) && buffer)
                        {
                            resultValue = buffer->QueryInterface(returnObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, stride, count, type, flags, staticData));
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(this, std::bind([this](LPCVOID handle, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> VideoObjectPtr
                {
                    CComPtr<VideoBuffer> buffer;
                    HRESULT resultValue = video->createBuffer(&buffer, stride, count, type, flags, staticData);
                    if (SUCCEEDED(resultValue) && buffer)
                    {
                        resultValue = buffer->QueryInterface(returnObject);
                    }

                    return resultValue;
                }, std::placeholders::_1, stride, count, type, flags, staticData));
            }
        }

        ResourceHandle createBuffer(const wchar_t *name, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData)
        {
            if (name)
            {
                std::size_t hash = std::hash<const wchar_t *>()(name);
                if (flags & TextureFlags::ReadWrite ? true : false)
                {
                    return resourceManager.getReadWriteHandle(hash, this, std::bind([this](LPCVOID handle, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> VideoObjectPtr
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
                    }, std::placeholders::_1, format, count, type, flags, staticData));
                }
                else
                {
                    return resourceManager.getHandle(hash, this, std::bind([this](LPCVOID handle, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> VideoObjectPtr
                    {
                        CComPtr<VideoBuffer> buffer;
                        HRESULT resultValue = video->createBuffer(&buffer, format, count, type, flags, staticData);
                        if (SUCCEEDED(resultValue) && buffer)
                        {
                            resultValue = buffer->QueryInterface(returnObject);
                        }

                        return resultValue;
                    }, std::placeholders::_1, format, count, type, flags, staticData));
                }
            }
            else
            {
                return resourceManager.getGlobalHandle(this, std::bind([this](LPCVOID handle, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData) -> VideoObjectPtr
                {
                    CComPtr<VideoBuffer> buffer;
                    HRESULT resultValue = video->createBuffer(&buffer, format, count, type, flags, staticData);
                    if (SUCCEEDED(resultValue) && buffer)
                    {
                        resultValue = buffer->QueryInterface(returnObject);
                    }

                    return resultValue;
                }, std::placeholders::_1, format, count, type, flags, staticData));
            }
        }

        void createTexture(VideoTexture **returnObject, wstring parameters, UINT32 flags)
        {
            GEK_REQUIRE(returnObject);

            HRESULT resultValue = E_FAIL;

            CComPtr<VideoTexture> texture;

            int position = 0;
            wstring type(parameters.Tokenize(L":", position));
            wstring value(parameters.Tokenize(L":", position));
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

        void loadTexture(VideoTexture **returnObject, const wchar_t *fileName, UINT32 flags)
        {
            GEK_REQUIRE(returnObject);
            GEK_REQUIRE(fileName);

            HRESULT resultValue = E_FAIL;

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

            CComPtr<VideoTexture> texture;
            for (auto &format : formatList)
            {
                wstring fullFileName(FileSystem::expandPath(String::format(L"$root\\data\\textures\\%%", fileName, format)));
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

        ResourceHandle loadTexture(const wchar_t *fileName, const wchar_t *fallback, UINT32 flags)
        {
            std::size_t hash = std::hash<wstring>()(wstring(fileName).MakeReverse());
            return resourceManager.getHandle(hash, this, std::bind([this](LPCVOID handle, const wstring &fileName, const wstring &fallback, UINT32 flags) -> VideoObjectPtr
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
            }, std::placeholders::_1, fileName, fallback, flags));
        }

        ProgramHandle loadComputeProgram(const wchar_t *fileName, const char *entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude, const std::unordered_map<string, string> &defineList)
        {
            return programManager.getUniqueHandle(this, std::bind([this](LPCVOID handle, const wstring &fileName, const string &entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude, std::unordered_map<string, string> defineList) -> VideoObjectPtr
            {
                return video->loadComputeProgram(fileName, entryFunction, onInclude, defineList);
            }, std::placeholders::_1, fileName, entryFunction, onInclude, defineList));
        }

        ProgramHandle loadPixelProgram(const wchar_t *fileName, const char *entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude, const std::unordered_map<string, string> &defineList)
        {
            return programManager.getUniqueHandle(this, std::bind([this](LPCVOID handle, const wstring &fileName, const string &entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude, std::unordered_map<string, string> defineList) -> VideoObjectPtr
            {
                return video->loadPixelProgram(fileName, entryFunction, onInclude, defineList);
            }, std::placeholders::_1, fileName, entryFunction, onInclude, defineList));
        }

        void mapBuffer(ResourceHandle buffer, void **data)
        {
            video->mapBuffer(resourceManager.getResource<VideoBuffer>(buffer), data);
        }

        void unmapBuffer(ResourceHandle buffer)
        {
            video->unmapBuffer(resourceManager.getResource<VideoBuffer>(buffer));
        }

        void flip(ResourceHandle resourceHandle)
        {
            resourceManager.flipResource(resourceHandle);
        }

        void generateMipMaps(RenderContext *renderContext, ResourceHandle resourceHandle)
        {
            renderContext->getContext()->generateMipMaps(resourceManager.getResource<VideoTexture>(resourceHandle));
        }

        void copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle)
        {
            video->copyResource(resourceManager.getResource(destinationHandle), resourceManager.getResource(sourceHandle));
        }

        void setRenderState(RenderContext *renderContext, RenderStateHandle renderStateHandle)
        {
            renderContext->getContext()->setRenderState(renderStateManager.getResource<IUnknown>(renderStateHandle));
        }

        void setDepthState(RenderContext *renderContext, DepthStateHandle depthStateHandle, UINT32 stencilReference)
        {
            renderContext->getContext()->setDepthState(depthStateManager.getResource<IUnknown>(depthStateHandle), stencilReference);
        }

        void setBlendState(RenderContext *renderContext, BlendStateHandle blendStateHandle, const Math::Color &blendFactor, UINT32 sampleMask)
        {
            renderContext->getContext()->setBlendState(blendStateManager.getResource<IUnknown>(blendStateHandle), blendFactor, sampleMask);
        }

        void setResource(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setResource(resourceManager.getResource<IUnknown>(resourceHandle), stage);
        }

        void setUnorderedAccess(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setUnorderedAccess(resourceManager.getResource<IUnknown>(resourceHandle, true), stage);
        }

        void setConstantBuffer(RenderPipeline *renderPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            renderPipeline->getPipeline()->setConstantBuffer(resourceManager.getResource<VideoBuffer>(resourceHandle), stage);
        }

        void setProgram(RenderPipeline *renderPipeline, ProgramHandle programHandle)
        {
            renderPipeline->getPipeline()->setProgram(programManager.getResource<IUnknown>(programHandle));
        }

        void setVertexBuffer(RenderContext *renderContext, UINT32 slot, ResourceHandle resourceHandle, UINT32 offset)
        {
            renderContext->getContext()->setVertexBuffer(slot, resourceManager.getResource<VideoBuffer>(resourceHandle), offset);
        }

        void setIndexBuffer(RenderContext *renderContext, ResourceHandle resourceHandle, UINT32 offset)
        {
            renderContext->getContext()->setIndexBuffer(resourceManager.getResource<VideoBuffer>(resourceHandle), offset);
        }

        void clearRenderTarget(RenderContext *renderContext, ResourceHandle resourceHandle, const Math::Color &color)
        {
            renderContext->getContext()->clearRenderTarget(resourceManager.getResource<VideoTarget>(resourceHandle, true), color);
        }

        void clearDepthStencilTarget(RenderContext *renderContext, ResourceHandle depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear)
        {
            renderContext->getContext()->clearDepthStencilTarget(resourceManager.getResource<IUnknown>(depthBuffer), flags, depthClear, stencilClear);
        }

        Video::ViewPort viewPortList[8];
        VideoTarget *renderTargetList[8];
        void setRenderTargets(RenderContext *renderContext, ResourceHandle *renderTargetHandleList, UINT32 renderTargetHandleCount, ResourceHandle *depthBuffer)
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

        void setBackBuffer(RenderContext *renderContext, ResourceHandle *depthBuffer)
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
