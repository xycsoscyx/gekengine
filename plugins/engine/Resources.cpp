#include "GEK\Engine\Render.h"
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
#include "GEK\Context\Common.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shapes\Sphere.h"
#include <concurrent_unordered_map.h>
#include <ppl.h>
#include <set>

namespace Gek
{
    static const UINT32 MaxLightCount = 255;

    template <class HANDLE>
    class ObjectManager
    {
    public:
        struct ReadWrite
        {
            CComPtr<IUnknown> read;
            CComPtr<IUnknown> write;

            ReadWrite(void)
            {
            }

            ReadWrite(CComPtr<IUnknown> read, CComPtr<IUnknown> write)
                : read(read)
                , write(write)
            {
            }
        };

    private:
        UINT64 nextIdentifier;
        concurrency::concurrent_unordered_map<std::size_t, HANDLE> hashMap;
        concurrency::concurrent_unordered_map<HANDLE, CComPtr<IUnknown>> localResourceMap;
        concurrency::concurrent_unordered_map<HANDLE, ReadWrite> readWriteResourceMap;
        concurrency::concurrent_unordered_map<HANDLE, CComPtr<IUnknown>> globalResourceMap;

    public:
        ObjectManager(void)
            : nextIdentifier(0)
        {
        }

        ~ObjectManager(void)
        {
        }

        void clearResources(void)
        {
            hashMap.clear();
            localResourceMap.clear();
            readWriteResourceMap.clear();
        }

        HANDLE getUniqueResourceHandle(std::function<HRESULT(IUnknown **)> loadResource, bool readWrite = false)
        {
            HANDLE handle;
            handle.assign(InterlockedIncrement(&nextIdentifier));
            if (readWrite)
            {
                readWriteResourceMap[handle] = ReadWrite();

                CComPtr<IUnknown> read, write;
                if (SUCCEEDED(loadResource(&read)) && read &&
                    SUCCEEDED(loadResource(&write)) && write)
                {
                    readWriteResourceMap[handle] = ReadWrite(read, write);
                }
            }
            else
            {
                localResourceMap[handle] = nullptr;

                CComPtr<IUnknown> resource;
                if (SUCCEEDED(loadResource(&resource)) && resource)
                {
                    localResourceMap[handle] = resource;
                }
            }

            return handle;
        }

        HANDLE getResourceHandle(std::size_t hash, std::function<HRESULT(IUnknown **)> loadResource, bool readWrite = false)
        {
            HANDLE handle;
            if (hash == 0)
            {
                CComPtr<IUnknown> resource;
                if (SUCCEEDED(loadResource(&resource)) && resource)
                {
                    handle.assign(InterlockedIncrement(&nextIdentifier));
                    globalResourceMap[handle] = resource;
                }
            }
            else
            {
                auto hashIterator = hashMap.find(hash);
                if (hashIterator == hashMap.end())
                {
                    handle.assign(InterlockedIncrement(&nextIdentifier));
                    hashMap[hash] = handle;
                    if (readWrite)
                    {
                        readWriteResourceMap[handle] = ReadWrite();

                        CComPtr<IUnknown> read, write;
                        if (SUCCEEDED(loadResource(&read)) && read &&
                            SUCCEEDED(loadResource(&write)) && write)
                        {
                            readWriteResourceMap[handle] = ReadWrite(read, write);
                        }
                    }
                    else
                    {
                        localResourceMap[handle] = nullptr;

                        CComPtr<IUnknown> resource;
                        if (SUCCEEDED(loadResource(&resource)) && resource)
                        {
                            localResourceMap[handle] = resource;
                        }
                    }
                }
                else
                {
                    handle = hashIterator->second;
                }
            }

            return handle;
        }

        IUnknown *getResource(HANDLE handle, bool writable = false)
        {
            auto globalIterator = globalResourceMap.find(handle);
            if (globalIterator != globalResourceMap.end())
            {
                return globalIterator->second.p;
            }

            auto localIterator = localResourceMap.find(handle);
            if (localIterator != localResourceMap.end())
            {
                return localIterator->second.p;
            }

            auto readWriteIterator = readWriteResourceMap.find(handle);
            if (readWriteIterator != readWriteResourceMap.end())
            {
                auto &resource = readWriteIterator->second;
                return (writable ? resource.write : resource.read);
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
                std::swap(resource.read, resource.write);
            }
        }
    };

    class ResourcesImplementation : public ContextUserMixin
        , public Resources
    {
    private:
        IUnknown *initializerContext;
        VideoSystem *video;

        ObjectManager<ProgramHandle> programManager;
        ObjectManager<PluginHandle> pluginManager;
        ObjectManager<PropertiesHandle> materialManager;
        ObjectManager<ShaderHandle> shaderManager;
        ObjectManager<ResourceHandle> resourceManager;
        ObjectManager<RenderStatesHandle> renderStateManager;
        ObjectManager<DepthStatesHandle> depthStateManager;
        ObjectManager<BlendStatesHandle> blendStateManager;

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

        // Resources
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            REQUIRE_RETURN(initializerContext, E_INVALIDARG);

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
            programManager.clearResources();
            materialManager.clearResources();
            shaderManager.clearResources();
            resourceManager.clearResources();
            renderStateManager.clearResources();
            depthStateManager.clearResources();
            blendStateManager.clearResources();
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
            else if (type == typeid(PropertiesHandle))
            {
                return materialManager.getResource(*static_cast<const PropertiesHandle *>(handle));
            }

            return nullptr;
        };

        STDMETHODIMP_(PluginHandle) loadPlugin(LPCWSTR fileName)
        {
            std::size_t hash = std::hash<CStringW>()(CStringW(fileName).MakeReverse());
            return pluginManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
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
            });
        }

        STDMETHODIMP_(MaterialHandle) loadMaterial(LPCWSTR fileName)
        {
            ShaderHandle shader;
            std::size_t hash = std::hash<CStringW>()(CStringW(fileName).MakeReverse());
            PropertiesHandle properties = materialManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                CComPtr<Material> material;
                resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(MaterialRegistration, &material));
                if (material)
                {
                    resultValue = material->initialize(initializerContext, fileName);
                    if (SUCCEEDED(resultValue))
                    {
                        shader = material->getShader();
                        resultValue = material->QueryInterface(returnObject);
                    }
                }

                return resultValue;
            });

            return MaterialHandle(shader, properties);
        }

        STDMETHODIMP_(ShaderHandle) loadShader(LPCWSTR fileName)
        {
            std::size_t hash = std::hash<CStringW>()(CStringW(fileName).MakeReverse());
            return shaderManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
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
            });
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
            return renderStateManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = video->createRenderStates(returnObject, renderStates);
                return resultValue;
            });
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
            return depthStateManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = video->createDepthStates(returnObject, depthStates);
                return resultValue;
            });
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
            return blendStateManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = video->createBlendStates(returnObject, blendStates);
                return resultValue;
            });
        }

        STDMETHODIMP_(BlendStatesHandle) createBlendStates(const Video::IndependentBlendStates &blendStates)
        {
            std::size_t hash = 0;
            for (UINT32 renderTarget = 0; renderTarget < 8; ++renderTarget)
            {
                std::hash_combine(hash, std::hash_combine(blendStates.targetStates[renderTarget].enable,
                    static_cast<UINT8>(blendStates.targetStates[renderTarget].colorSource),
                    static_cast<UINT8>(blendStates.targetStates[renderTarget].colorDestination),
                    static_cast<UINT8>(blendStates.targetStates[renderTarget].colorOperation),
                    static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaSource),
                    static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaDestination),
                    static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaOperation),
                    blendStates.targetStates[renderTarget].writeMask));
            }

            return blendStateManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = video->createBlendStates(returnObject, blendStates);
                return resultValue;
            });
        }

        STDMETHODIMP_(ResourceHandle) createTexture(LPCWSTR name, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, DWORD flags)
        {
            std::size_t hash = (name ? std::hash<LPCWSTR>()(name) : 0);
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                CComPtr<VideoTexture> texture;
                HRESULT resultValue = video->createTexture(&texture, format, width, height, depth, flags);
                if (SUCCEEDED(resultValue) && texture)
                {
                    resultValue = texture->QueryInterface(returnObject);
                }

                return resultValue;
            }, (flags & TextureFlags::ReadWrite ? true : false));
        }

        STDMETHODIMP_(ResourceHandle) createBuffer(LPCWSTR name, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData)
        {
            std::size_t hash = (name ? std::hash<LPCWSTR>()(name) : 0);
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                CComPtr<VideoBuffer> buffer;
                HRESULT resultValue = video->createBuffer(&buffer, stride, count, type, flags, staticData);
                if (SUCCEEDED(resultValue) && buffer)
                {
                    resultValue = buffer->QueryInterface(returnObject);
                }

                return resultValue;
            }, (flags & TextureFlags::ReadWrite ? true : false));
        }

        STDMETHODIMP_(ResourceHandle) createBuffer(LPCWSTR name, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData)
        {
            std::size_t hash = (name ? std::hash<LPCWSTR>()(name) : 0);
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                CComPtr<VideoBuffer> buffer;
                HRESULT resultValue = video->createBuffer(&buffer, format, count, type, flags, staticData);
                if (SUCCEEDED(resultValue) && buffer)
                {
                    if (flags & Video::BufferFlags::UnorderedAccess)
                    {
                        video->getDefaultContext()->clearUnorderedAccessBuffer(buffer, 1.0f);
                    }

                    resultValue = buffer->QueryInterface(returnObject);
                }

                return resultValue;
            }, (flags & TextureFlags::ReadWrite ? true : false));
        }

        HRESULT createTexture(VideoTexture **returnObject, CStringW parameters, UINT32 flags)
        {
            REQUIRE_RETURN(returnObject, E_INVALIDARG);

            gekCheckScope(resultValue, parameters, flags);

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
                    UINT32 normalValue = UINT32(UINT8(normal.x * 255.0f)) |
                                         UINT32(UINT8(normal.y * 255.0f) << 8) |
                                         UINT32(UINT8(normal.z * 255.0f) << 16);
                    video->updateTexture(texture, &normalValue, 4);
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
            REQUIRE_RETURN(returnObject, E_INVALIDARG);
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, flags);

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

        STDMETHODIMP_(ResourceHandle) loadTexture(LPCWSTR fileName, UINT32 flags)
        {
            std::size_t hash = std::hash<CStringW>()(CStringW(fileName).MakeReverse());
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                REQUIRE_RETURN(fileName, E_INVALIDARG);

                gekCheckScope(resultValue, fileName, flags);

                CComPtr<VideoTexture> texture;
                if ((*fileName) && (*fileName) == L'*')
                {
                    resultValue = createTexture(&texture, &fileName[1], flags);
                }
                else
                {
                    resultValue = loadTexture(&texture, fileName, flags);
                }

                if (texture)
                {
                    resultValue = texture->QueryInterface(returnObject);
                }

                return resultValue;
            });
        }

        STDMETHODIMP_(ProgramHandle) loadComputeProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            return programManager.getUniqueResourceHandle([&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = video->loadComputeProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                return resultValue;
            });
        }

        STDMETHODIMP_(ProgramHandle) loadPixelProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            return programManager.getUniqueResourceHandle([&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;
                resultValue = video->loadPixelProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                return resultValue;
            });
        }

        STDMETHODIMP mapBuffer(ResourceHandle buffer, LPVOID *data)
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

        STDMETHODIMP_(void) generateMipMaps(VideoContext *videoContext, ResourceHandle resourceHandle)
        {
            videoContext->generateMipMaps(resourceManager.getResource<VideoTexture>(resourceHandle));
        }

        STDMETHODIMP_(void) copyResource(ResourceHandle destinationHandle, ResourceHandle sourceHandle)
        {
            video->copyResource(resourceManager.getResource(destinationHandle), resourceManager.getResource(sourceHandle));
        }

        STDMETHODIMP_(void) setRenderStates(VideoContext *videoContext, RenderStatesHandle renderStatesHandle)
        {
            videoContext->setRenderStates(renderStateManager.getResource<IUnknown>(renderStatesHandle));
        }

        STDMETHODIMP_(void) setDepthStates(VideoContext *videoContext, DepthStatesHandle depthStatesHandle, UINT32 stencilReference)
        {
            videoContext->setDepthStates(depthStateManager.getResource<IUnknown>(depthStatesHandle), stencilReference);
        }

        STDMETHODIMP_(void) setBlendStates(VideoContext *videoContext, BlendStatesHandle blendStatesHandle, const Math::Color &blendFactor, UINT32 sampleMask)
        {
            videoContext->setBlendStates(blendStateManager.getResource<IUnknown>(blendStatesHandle), blendFactor, sampleMask);
        }

        STDMETHODIMP_(void) setResource(VideoPipeline *videoPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            videoPipeline->setResource(resourceManager.getResource<IUnknown>(resourceHandle), stage);
        }

        STDMETHODIMP_(void) setUnorderedAccess(VideoPipeline *videoPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            videoPipeline->setUnorderedAccess(resourceManager.getResource<IUnknown>(resourceHandle, true), stage);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoPipeline *videoPipeline, ResourceHandle resourceHandle, UINT32 stage)
        {
            videoPipeline->setConstantBuffer(resourceManager.getResource<VideoBuffer>(resourceHandle), stage);
        }

        STDMETHODIMP_(void) setProgram(VideoPipeline *videoPipeline, ProgramHandle programHandle)
        {
            videoPipeline->setProgram(programManager.getResource<IUnknown>(programHandle));
        }

        STDMETHODIMP_(void) setVertexBuffer(VideoContext *videoContext, UINT32 slot, ResourceHandle resourceHandle, UINT32 offset)
        {
            videoContext->setVertexBuffer(slot, resourceManager.getResource<VideoBuffer>(resourceHandle), offset);
        }

        STDMETHODIMP_(void) setIndexBuffer(VideoContext *videoContext, ResourceHandle resourceHandle, UINT32 offset)
        {
            videoContext->setIndexBuffer(resourceManager.getResource<VideoBuffer>(resourceHandle), offset);
        }

        STDMETHODIMP_(void) clearRenderTarget(VideoContext *videoContext, ResourceHandle resourceHandle, const Math::Color &color)
        {
            videoContext->clearRenderTarget(resourceManager.getResource<VideoTarget>(resourceHandle, true), color);
        }

        STDMETHODIMP_(void) clearDepthStencilTarget(VideoContext *videoContext, ResourceHandle depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear)
        {
            videoContext->clearDepthStencilTarget(resourceManager.getResource<IUnknown>(depthBuffer), flags, depthClear, stencilClear);
        }

        STDMETHODIMP_(void) setRenderTargets(VideoContext *videoContext, ResourceHandle *renderTargetHandleList, UINT32 renderTargetHandleCount, ResourceHandle depthBuffer)
        {
            static Video::ViewPort viewPortList[8];
            static VideoTarget *renderTargetList[8];
            for (UINT32 renderTarget = 0; renderTarget < renderTargetHandleCount; renderTarget++)
            {
                renderTargetList[renderTarget] = resourceManager.getResource<VideoTarget>(renderTargetHandleList[renderTarget], true);
                viewPortList[renderTarget] = renderTargetList[renderTarget]->getViewPort();
            }

            videoContext->setRenderTargets(renderTargetList, renderTargetHandleCount, resourceManager.getResource<IUnknown>(depthBuffer));
            videoContext->setViewports(viewPortList, renderTargetHandleCount);
        }

        STDMETHODIMP_(void) setDefaultTargets(VideoContext *videoContext, ResourceHandle depthBuffer)
        {
            video->setDefaultTargets(videoContext, resourceManager.getResource<IUnknown>(depthBuffer));
        }

    };

    REGISTER_CLASS(ResourcesImplementation)
}; // namespace Gek
