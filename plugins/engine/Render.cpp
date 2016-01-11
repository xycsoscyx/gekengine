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
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shape\Sphere.h"
#include <set>
#include <ppl.h>

namespace Gek
{
    static const UINT32 MaxLightCount = 255;

    template <class HANDLE>
    class ObjectManager
    {
    private:
        UINT64 nextIdentifier;
        std::unordered_map<std::size_t, HANDLE> hashMap;
        std::unordered_map<HANDLE, CComPtr<IUnknown>> localResourceMap;
        std::unordered_map<HANDLE, CComPtr<IUnknown>> globalResourceMap;

    public:
        ObjectManager(void)
            : nextIdentifier(0)
        {
        }

        ~ObjectManager(void)
        {
        }

        BEGIN_INTERFACE_LIST(ObjectManager)
        END_INTERFACE_LIST_USER

        void clearResources(void)
        {
            hashMap.clear();
            localResourceMap.clear();
        }

        HANDLE getResourceHandle(std::size_t hash, std::function<HRESULT(IUnknown **)> loadResource)
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
                    localResourceMap[handle] = nullptr;

                    CComPtr<IUnknown> resource;
                    if (SUCCEEDED(loadResource(&resource)) && resource)
                    {
                        localResourceMap[handle] = resource;
                    }
                }
                else
                {
                    handle = hashIterator->second;
                }
            }

            return handle;
        }

        HANDLE addUniqueResource(IUnknown *resource)
        {
            HANDLE handle;
            if (resource)
            {
                handle.assign(InterlockedIncrement(&nextIdentifier));
                localResourceMap[handle] = resource;
            }

            return handle;
        }

        template <typename TYPE>
        TYPE *getResource(HANDLE handle)
        {
            auto globalIterator = globalResourceMap.find(handle);
            if (globalIterator != globalResourceMap.end())
            {
                return dynamic_cast<TYPE *>(globalIterator->second.p);
            }

            auto localIterator = localResourceMap.find(handle);
            if (localIterator != localResourceMap.end())
            {
                return dynamic_cast<TYPE *>(localIterator->second.p);
            }

            return nullptr;
        }
    };

    class RenderImplementation : public ContextUserMixin
        , public ObservableMixin
        , public PopulationObserver
        , public Resources
        , public Render
    {
    public:
        struct CameraConstantData
        {
            Math::Float2 fieldOfView;
            float minimumDistance;
            float maximumDistance;
            Math::Float4x4 viewMatrix;
            Math::Float4x4 projectionMatrix;
            Math::Float4x4 inverseProjectionMatrix;
        };

        struct LightConstants
        {
            UINT32 count;
            UINT32 padding[3];
        };

        struct LightData
        {
            Math::Float3 position;
            float range;
            Math::Float3 color;
            float distance;
            LightData(const Math::Float3 &position, float range, const Math::Float3 &color)
                : position(position)
                , range(range)
                , color(color)
                , distance(position.getLength())
            {
            }
        };

        typedef std::function<void(VideoContext *)> DrawCall;
        struct DrawCallValue
        {
            union
            {
                UINT64 value;
                struct
                {
                    ShaderHandle shader;
                    PluginHandle plugin;
                    MaterialHandle material;
                };
            };

            DrawCall onDraw;

            DrawCallValue(ShaderHandle shader, PluginHandle plugin, MaterialHandle material, DrawCall onDraw)
                : shader(shader)
                , plugin(plugin)
                , material(material)
                , onDraw(onDraw)
            {
            }
        };

    private:
        IUnknown *initializerContext;
        VideoSystem *video;
        Population *population;
        UINT32 updateHandle;

        CComPtr<IUnknown> pointSamplerStates;
        CComPtr<IUnknown> linearSamplerStates;
        CComPtr<VideoBuffer> cameraConstantBuffer;
        CComPtr<VideoBuffer> lightConstantBuffer;
        CComPtr<VideoBuffer> lightDataBuffer;

        CComPtr<IUnknown> deferredVertexProgram;

        ObjectManager<ProgramHandle> programManager;
        ObjectManager<PluginHandle> pluginManager;
        ObjectManager<MaterialHandle> materialManager;
        ObjectManager<ShaderHandle> shaderManager;
        ObjectManager<ResourceHandle> resourceManager;
        ObjectManager<RenderStatesHandle> renderStateManager;
        ObjectManager<DepthStatesHandle> depthStateManager;
        ObjectManager<BlendStatesHandle> blendStateManager;
        std::unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

        std::vector<LightData> lightList;
        std::vector<DrawCallValue> drawCallList;

    public:
        RenderImplementation(void)
            : initializerContext(nullptr)
            , video(nullptr)
            , population(nullptr)
            , updateHandle(0)
        {
        }

        ~RenderImplementation(void)
        {
            population->removeUpdatePriority(updateHandle);
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(RenderImplementation)
            INTERFACE_LIST_ENTRY_COM(Observable)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(PluginResources)
            INTERFACE_LIST_ENTRY_COM(Resources)
            INTERFACE_LIST_ENTRY_COM(Render)
        END_INTERFACE_LIST_USER

        // Render/Resources
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<VideoSystem> video(initializerContext);
            CComQIPtr<Population> population(initializerContext);
            if (video && population)
            {
                this->video = video;
                this->population = population;
                this->initializerContext = initializerContext;
                resultValue = ObservableMixin::addObserver(population, getClass<PopulationObserver>());
                updateHandle = population->setUpdatePriority(this, 100);
            }

            if (SUCCEEDED(resultValue))
            {
                Video::SamplerStates samplerStates;
                samplerStates.filterMode = Video::FilterMode::AllPoint;
                samplerStates.addressModeU = Video::AddressMode::Clamp;
                samplerStates.addressModeV = Video::AddressMode::Clamp;
                resultValue = video->createSamplerStates(&pointSamplerStates, samplerStates);
            }

            if (SUCCEEDED(resultValue))
            {
                Video::SamplerStates samplerStates;
                samplerStates.maximumAnisotropy = 8;
                samplerStates.filterMode = Video::FilterMode::Anisotropic;
                samplerStates.addressModeU = Video::AddressMode::Wrap;
                samplerStates.addressModeV = Video::AddressMode::Wrap;
                resultValue = video->createSamplerStates(&linearSamplerStates, samplerStates);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&cameraConstantBuffer, sizeof(CameraConstantData), 1, Video::BufferType::Constant, 0);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&lightConstantBuffer, sizeof(LightConstants), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&lightDataBuffer, sizeof(LightData), MaxLightCount, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
            }

            if (SUCCEEDED(resultValue))
            {
                static const char program[] =
                    "struct Pixel                                                                       \r\n" \
                    "{                                                                                  \r\n" \
                    "    float4 position : SV_POSITION;                                                 \r\n" \
                    "    float2 texCoord : TEXCOORD0;                                                   \r\n" \
                    "};                                                                                 \r\n" \
                    "                                                                                   \r\n" \
                    "Pixel mainVertexProgram(in uint vertexID : SV_VertexID)                            \r\n" \
                    "{                                                                                  \r\n" \
                    "    Pixel pixel;                                                                   \r\n" \
                    "    pixel.texCoord = float2((vertexID << 1) & 2, vertexID & 2);                    \r\n" \
                    "    pixel.position = float4(pixel.texCoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f); \r\n" \
                    "    return pixel;                                                                  \r\n" \
                    "}                                                                                  \r\n" \
                    "                                                                                   \r\n";

                resultValue = video->compileVertexProgram(&deferredVertexProgram, program, "mainVertexProgram", nullptr);
            }

            return resultValue;
        }

        // Resources
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
            MaterialHandle material = materialManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
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

            if (shader.isValid())
            {
                materialShaderMap[material] = shader;
            }

            return material;
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

        STDMETHODIMP_(void) loadResourceList(ShaderHandle shaderHandle, LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::vector<ResourceHandle> &resourceList)
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
                HRESULT resultValue = E_FAIL;
                resultValue = video->createRenderStates(returnObject, renderStates);
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
                HRESULT resultValue = E_FAIL;
                resultValue = video->createDepthStates(returnObject, depthStates);
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
                HRESULT resultValue = E_FAIL;
                resultValue = video->createBlendStates(returnObject, blendStates);
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
                HRESULT resultValue = E_FAIL;
                resultValue = video->createBlendStates(returnObject, blendStates);
                return resultValue;
            });
        }

        STDMETHODIMP_(ResourceHandle) createRenderTarget(Video::Format format, UINT32 width, UINT32 height, UINT32 flags)
        {
            CComPtr<VideoTarget> renderTarget;
            video->createRenderTarget(&renderTarget, format, width, height, flags);
            return resourceManager.addUniqueResource(renderTarget);
        }

        STDMETHODIMP_(ResourceHandle) createDepthTarget(Video::Format format, UINT32 width, UINT32 height, UINT32 flags)
        {
            CComPtr<IUnknown> depthTarget;
            video->createDepthTarget(&depthTarget, format, width, height, flags);
            return resourceManager.addUniqueResource(depthTarget);
        }

        STDMETHODIMP_(ResourceHandle) createBuffer(LPCWSTR name, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData)
        {
            std::size_t hash = (name ? std::hash<LPCWSTR>()(name) : 0);
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                CComPtr<VideoBuffer> buffer;
                resultValue = video->createBuffer(&buffer, stride, count, type, flags, staticData);
                if (SUCCEEDED(resultValue) && buffer)
                {
                    resultValue = buffer->QueryInterface(returnObject);
                }

                return resultValue;
            });
        }

        STDMETHODIMP_(ResourceHandle) createBuffer(LPCWSTR name, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData)
        {
            std::size_t hash = (name ? std::hash<LPCWSTR>()(name) : 0);
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                CComPtr<VideoBuffer> buffer;
                resultValue = video->createBuffer(&buffer, format, count, type, flags, staticData);
                if (SUCCEEDED(resultValue) && buffer)
                {
                    resultValue = buffer->QueryInterface(returnObject);
                }

                return resultValue;
            });
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
                    Math::Float3 normal((String::to<Math::Float3>(value) + 1.0f) * 0.5f);
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
            for (auto format : formatList)
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
            std::size_t hash = std::hash_combine(fileName, entryFunction);
            if (defineList)
            {
                for (auto &define : (*defineList))
                {
                    hash = std::hash_combine(hash, std::hash_combine(define.first, define.second));
                }
            }

            return programManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;
                resultValue = video->loadComputeProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                return resultValue;
            });
        }

        STDMETHODIMP_(ProgramHandle) loadPixelProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            std::size_t hash = std::hash_combine(fileName, entryFunction);
            if (defineList)
            {
                for (auto &define : (*defineList))
                {
                    hash = std::hash_combine(hash, std::hash_combine(define.first, define.second));
                }
            }

            return programManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
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

        STDMETHODIMP_(void) generateMipMaps(VideoContext *videoContext, ResourceHandle resourceHandle)
        {
            videoContext->generateMipMaps(resourceManager.getResource<VideoTexture>(resourceHandle));
        }

        STDMETHODIMP_(void) setRenderStates(VideoContext *videoContext, RenderStatesHandle renderStatesHandle)
        {
            videoContext->setRenderStates(renderStateManager.getResource<IUnknown>(renderStatesHandle));
        }

        STDMETHODIMP_(void) setDepthStates(VideoContext *videoContext, DepthStatesHandle depthStatesHandle, UINT32 stencilReference)
        {
            videoContext->setDepthStates(depthStateManager.getResource<IUnknown>(depthStatesHandle), stencilReference);
        }

        STDMETHODIMP_(void) setBlendStates(VideoContext *videoContext, BlendStatesHandle blendStatesHandle, const Math::Float4 &blendFactor, UINT32 sampleMask)
        {
            videoContext->setBlendStates(blendStateManager.getResource<IUnknown>(blendStatesHandle), blendFactor, sampleMask);
        }

        STDMETHODIMP_(void) setResource(VideoPipeline *videoPipeline, ResourceHandle ResourceHandle, UINT32 stage)
        {
            videoPipeline->setResource(resourceManager.getResource<IUnknown>(ResourceHandle), stage);
        }

        STDMETHODIMP_(void) setUnorderedAccess(VideoPipeline *videoPipeline, ResourceHandle ResourceHandle, UINT32 stage)
        {
            videoPipeline->setUnorderedAccess(resourceManager.getResource<IUnknown>(ResourceHandle), stage);
        }

        STDMETHODIMP_(void) setProgram(VideoPipeline *videoPipeline, ProgramHandle programHandle)
        {
            videoPipeline->setProgram(programManager.getResource<IUnknown>(programHandle));
        }

        STDMETHODIMP_(void) setVertexBuffer(VideoContext *videoContext, UINT32 slot, ResourceHandle ResourceHandle, UINT32 offset)
        {
            videoContext->setVertexBuffer(slot, resourceManager.getResource<VideoBuffer>(ResourceHandle), offset);
        }

        STDMETHODIMP_(void) setIndexBuffer(VideoContext *videoContext, ResourceHandle ResourceHandle, UINT32 offset)
        {
            videoContext->setIndexBuffer(resourceManager.getResource<VideoBuffer>(ResourceHandle), offset);
        }

        STDMETHODIMP_(void) clearRenderTarget(VideoContext *videoContext, ResourceHandle ResourceHandle, const Math::Float4 &color)
        {
            videoContext->clearRenderTarget(resourceManager.getResource<VideoTarget>(ResourceHandle), color);
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
                renderTargetList[renderTarget] = resourceManager.getResource<VideoTarget>(renderTargetHandleList[renderTarget]);
                viewPortList[renderTarget] = renderTargetList[renderTarget]->getViewPort();
            }

            videoContext->setRenderTargets(renderTargetList, renderTargetHandleCount, resourceManager.getResource<IUnknown>(depthBuffer));
            videoContext->setViewports(viewPortList, renderTargetHandleCount);
        }

        STDMETHODIMP_(void) setDefaultTargets(VideoContext *videoContext, ResourceHandle depthBuffer)
        {
            video->setDefaultTargets(videoContext, resourceManager.getResource<IUnknown>(depthBuffer));
        }

        // Render
        STDMETHODIMP_(void) queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(VideoContext *)> draw)
        {
            drawCallList.emplace_back(materialShaderMap[material], plugin, material, draw);
        }

        // PopulationObserver
        STDMETHODIMP_(void) onLoadBegin(void)
        {
        }

        STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
        {
            if (FAILED(resultValue))
            {
                onFree();
            }
        }

        STDMETHODIMP_(void) onFree(void)
        {
            programManager.clearResources();
            materialManager.clearResources();
            shaderManager.clearResources();
            resourceManager.clearResources();
            renderStateManager.clearResources();
            depthStateManager.clearResources();
            blendStateManager.clearResources();
        }

        STDMETHODIMP_(void) onUpdate(float frameTime)
        {
            population->listEntities<TransformComponent, CameraComponent>([&](Entity *cameraEntity) -> void
            {
                auto &cameraTransform = cameraEntity->getComponent<TransformComponent>();
                Math::Float4x4 cameraMatrix(cameraTransform.getMatrix());

                auto &cameraData = cameraEntity->getComponent<CameraComponent>();

                CameraConstantData cameraConstantData;
                float displayAspectRatio = (1280.0f / 800.0f);
                float fieldOfView = Math::convertDegreesToRadians(cameraData.fieldOfView);
                cameraConstantData.fieldOfView.x = tan(fieldOfView * 0.5f);
                cameraConstantData.fieldOfView.y = (cameraConstantData.fieldOfView.x / displayAspectRatio);
                cameraConstantData.minimumDistance = cameraData.minimumDistance;
                cameraConstantData.maximumDistance = cameraData.maximumDistance;
                cameraConstantData.viewMatrix = cameraMatrix.getInverse();
                cameraConstantData.projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, cameraData.minimumDistance, cameraData.maximumDistance);
                cameraConstantData.inverseProjectionMatrix = cameraConstantData.projectionMatrix.getInverse();
                video->updateBuffer(cameraConstantBuffer, &cameraConstantData);

                VideoContext *videoContext = video->getDefaultContext();

                videoContext->geometryPipeline()->setConstantBuffer(cameraConstantBuffer, 0);
                videoContext->vertexPipeline()->setConstantBuffer(cameraConstantBuffer, 0);
                videoContext->pixelPipeline()->setConstantBuffer(cameraConstantBuffer, 0);
                videoContext->computePipeline()->setConstantBuffer(cameraConstantBuffer, 0);

                const Shape::Frustum viewFrustum(cameraConstantData.viewMatrix * cameraConstantData.projectionMatrix);

                lightList.clear();
                population->listEntities<TransformComponent, LightComponent, ColorComponent>([&](Entity *lightEntity) -> void
                {
                    auto &lightTransformComponent = lightEntity->getComponent<TransformComponent>();
                    auto &lightComponent = lightEntity->getComponent<LightComponent>();
                    if (viewFrustum.isVisible(Shape::Sphere(lightTransformComponent.position, lightComponent.range)))
                    {
                        auto &lightColorComponent = lightEntity->getComponent<ColorComponent>();
                        lightList.emplace_back((cameraConstantData.viewMatrix * lightTransformComponent.position.w(1.0f)).xyz, lightComponent.range, lightColorComponent.value.xyz);
                    }
                });

                drawCallList.clear();
                ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderScene, std::placeholders::_1, cameraEntity, &viewFrustum)));
                concurrency::parallel_sort(drawCallList.begin(), drawCallList.end(), [](const DrawCallValue &leftValue, const DrawCallValue &rightValue) -> bool
                {
                    return (leftValue.value < rightValue.value);
                });

                UINT32 lightListCount = lightList.size();
                videoContext->pixelPipeline()->setSamplerStates(pointSamplerStates, 0);
                videoContext->pixelPipeline()->setSamplerStates(linearSamplerStates, 1);
                videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);

                ShaderHandle currentShader;
                PluginHandle currentPlugin;
                MaterialHandle currentMaterial;
                UINT32 drawCallCount = drawCallList.size();
                for (UINT32 drawCallIndex = 0; drawCallIndex < drawCallCount; )
                {
                    DrawCallValue &drawCall = drawCallList[drawCallIndex];
                    Shader *shader = shaderManager.getResource<Shader>(currentShader = drawCall.shader);
                    auto drawLights = [&](std::function<void(void)> drawPasses) -> void
                    {
                        for (UINT32 lightBase = 0; lightBase < lightListCount; lightBase += MaxLightCount)
                        {
                            UINT32 lightCount = std::min((lightListCount - lightBase), MaxLightCount);

                            LightConstants *lightConstants = nullptr;
                            if (SUCCEEDED(video->mapBuffer(lightConstantBuffer, (LPVOID *)&lightConstants)))
                            {
                                memcpy(lightConstants, &lightCount, sizeof(UINT32));
                                video->unmapBuffer(lightConstantBuffer);
                            }

                            LightData *lightingData = nullptr;
                            if (SUCCEEDED(video->mapBuffer(lightDataBuffer, (LPVOID *)&lightingData)))
                            {
                                memcpy(lightingData, &lightList[lightBase], (sizeof(LightData) * lightCount));
                                video->unmapBuffer(lightDataBuffer);
                            }

                            drawPasses();
                        }
                    };

                    auto enableLights = [&](VideoPipeline *videoPipeline) -> void
                    {
                        videoPipeline->setResource(lightDataBuffer, 0);
                        videoPipeline->setConstantBuffer(lightConstantBuffer, 1);
                    };

                    auto drawForward = [&](void) -> void
                    {
                        while (drawCallIndex < drawCallCount)
                        {
                            drawCall = drawCallList[drawCallIndex++];
                            if (currentPlugin != drawCall.plugin)
                            {
                                Plugin *plugin = pluginManager.getResource<Plugin>(currentPlugin = drawCall.plugin);
                                if (plugin)
                                {
                                    plugin->enable(videoContext);
                                }
                            }

                            if (currentMaterial != drawCall.material)
                            {
                                Material *material = materialManager.getResource<Material>(currentMaterial = drawCall.material);
                                if (material)
                                {
                                    shader->setResourceList(videoContext, material->getResourceList());
                                }
                            }

                            drawCall.onDraw(videoContext);
                        };
                    };

                    auto drawDeferred = [&](void) -> void
                    {
                        videoContext->vertexPipeline()->setProgram(deferredVertexProgram);
                        videoContext->drawPrimitive(3, 0);
                    };

                    auto runCompute = [&](UINT32 dispatchWidth, UINT32 dispatchHeight, UINT32 dispatchDepth) -> void
                    {
                        videoContext->dispatch(dispatchWidth, dispatchHeight, dispatchDepth);
                    };

                    shader->draw(videoContext, drawLights, enableLights, drawForward, drawDeferred, runCompute);
                }
            });

            ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderOverlay, std::placeholders::_1)));

            video->present(true);
        }
    };

    REGISTER_CLASS(RenderImplementation)
}; // namespace Gek
