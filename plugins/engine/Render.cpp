﻿#include "GEK\Engine\Render.h"
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
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shape\Sphere.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace std
{
    template <>
    struct hash<LPCSTR>
    {
        size_t operator()(const LPCSTR &value) const
        {
            return hash<string>()(string(value));
        }
    };

    template <>
    struct hash<LPCWSTR>
    {
        size_t operator()(const LPCWSTR &value) const
        {
            return hash<wstring>()(wstring(value));
        }
    };

    inline size_t hashCombine(const size_t upper, const size_t lower)
    {
        return upper ^ (lower + 0x9e3779b9 + (upper << 6) + (upper >> 2));
    }

    inline size_t hashCombine(void)
    {
        return 0;
    }

    template <typename T, typename... Ts>
    size_t hashCombine(const T& t, const Ts&... ts)
    {
        size_t seed = hash<T>()(t);
        if (sizeof...(ts) == 0)
        {
            return seed;
        }

        size_t remainder = hashCombine(ts...);
        return hashCombine(seed, remainder);
    }

    template <typename CLASS>
    struct hash<CComPtr<CLASS>>
    {
        size_t operator()(const CComPtr<CLASS> &value) const
        {
            return hash<LPCVOID>()(value.p);
        }
    };

    template <typename CLASS>
    struct hash<CComQIPtr<CLASS>>
    {
        size_t operator()(const CComQIPtr<CLASS> &value) const
        {
            return hash<LPCVOID>()(value.p);
        }
    };
};

namespace Gek
{
    static const UINT32 MaxLightCount = 500;

    template <class HANDLE>
    class ObjectManager
    {
    private:
        UINT64 nextIdentifier;
        concurrency::concurrent_unordered_map<std::size_t, HANDLE> hashMap;
        concurrency::concurrent_unordered_map<HANDLE, CComPtr<IUnknown>> resourceMap;

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
            resourceMap.clear();
        }

        HANDLE getResourceHandle(std::size_t hash, std::function<HRESULT(IUnknown **)> loadResource)
        {
            HANDLE handle;
            auto hashIterator = hashMap.find(hash);
            if (hashIterator == hashMap.end())
            {
                handle = InterlockedIncrement(&nextIdentifier);
                hashMap[hash] = handle;
                resourceMap[handle] = nullptr;

                CComPtr<IUnknown> resource;
                if (SUCCEEDED(loadResource(&resource)) && resource)
                {
                    resourceMap[handle] = resource;
                }
            }
            else
            {
                handle = hashIterator->second;
            }

            return handle;
        }

        HANDLE addUniqueResource(IUnknown *resource)
        {
            HANDLE handle;
            if (resource)
            {
                handle = InterlockedIncrement(&nextIdentifier);
                resourceMap[handle] = resource;
            }

            return handle;
        }

        template <typename TYPE>
        TYPE *getResource(HANDLE handle)
        {
            auto resourceIterator = resourceMap.find(handle);
            if (resourceIterator != resourceMap.end())
            {
                return dynamic_cast<TYPE *>(resourceIterator->second.p);
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

        struct LightingConstantData
        {
            UINT32 count;
            UINT32 padding[3];
        };

        struct Light
        {
            Math::Float3 position;
            float radius;
            Math::Float3 color;
            float distance;
        };

    private:
        IUnknown *initializerContext;
        VideoSystem *video;
        Population *population;

        CComPtr<IUnknown> pointSamplerStates;
        CComPtr<IUnknown> linearSamplerStates;
        CComPtr<VideoBuffer> cameraConstantBuffer;
        CComPtr<VideoBuffer> lightingConstantBuffer;
        CComPtr<VideoBuffer> lightingBuffer;

        CComPtr<IUnknown> deferredVertexProgram;
        CComPtr<VideoBuffer> deferredVertexBuffer;
        CComPtr<VideoBuffer> deferredIndexBuffer;

        ObjectManager<ProgramHandle> programManager;
        ObjectManager<PluginHandle> pluginManager;
        ObjectManager<MaterialHandle> materialManager;
        ObjectManager<ShaderHandle> shaderManager;
        ObjectManager<ResourceHandle> resourceManager;
        ObjectManager<RenderStatesHandle> renderStateManager;
        ObjectManager<DepthStatesHandle> depthStateManager;
        ObjectManager<BlendStatesHandle> blendStateManager;
        std::unordered_map<MaterialHandle, ShaderHandle> materialShaderMap;

        std::unordered_map < ShaderHandle,
            std::unordered_map < PluginHandle,
            std::unordered_map < MaterialHandle,
            std::vector < std::function<void(VideoContext *)> > > > > queuedDrawCalls;

    public:
        RenderImplementation(void)
            : initializerContext(nullptr)
            , video(nullptr)
            , population(nullptr)
        {
        }

        ~RenderImplementation(void)
        {
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
                resultValue = video->createBuffer(&cameraConstantBuffer, sizeof(CameraConstantData), 1, Video::BufferFlags::ConstantBuffer);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&lightingConstantBuffer, sizeof(LightingConstantData), 1, Video::BufferFlags::ConstantBuffer);
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = video->createBuffer(&lightingBuffer, sizeof(Light), MaxLightCount, Video::BufferFlags::Dynamic | Video::BufferFlags::StructuredBuffer | Video::BufferFlags::Resource);
            }

            if (SUCCEEDED(resultValue))
            {
                static const char program[] =
                    "struct Vertex                                                                      \r\n" \
                    "{                                                                                  \r\n" \
                    "    float2 position : POSITION;                                                    \r\n" \
                    "    float2 texCoord : TEXCOORD0;                                                   \r\n" \
                    "};                                                                                 \r\n" \
                    "                                                                                   \r\n" \
                    "struct Pixel                                                                       \r\n" \
                    "{                                                                                  \r\n" \
                    "    float4 position : SV_POSITION;                                                 \r\n" \
                    "    float2 texCoord : TEXCOORD0;                                                   \r\n" \
                    "};                                                                                 \r\n" \
                    "                                                                                   \r\n" \
                    "Pixel mainVertexProgram(in Vertex vertex)                                          \r\n" \
                    "{                                                                                  \r\n" \
                    "    Pixel pixel;                                                                   \r\n" \
                    "    pixel.position = float4(vertex.position, 0.0f, 1.0f);                          \r\n" \
                    "    pixel.texCoord = vertex.texCoord;                                              \r\n" \
                    "    return pixel;                                                                  \r\n" \
                    "}                                                                                  \r\n" \
                    "                                                                                   \r\n";

                static const std::vector<Video::InputElement> elementList =
                {
                    Video::InputElement(Video::Format::Float2, "POSITION", 0),
                    Video::InputElement(Video::Format::Float2, "TEXCOORD", 0),
                };

                resultValue = video->compileVertexProgram(&deferredVertexProgram, program, "mainVertexProgram", elementList);
            }

            if (SUCCEEDED(resultValue))
            {
                static const Math::Float2 vertexList[] =
                {
                    Math::Float2(-1.0f, 1.0f), Math::Float2(0.0f, 0.0f),
                    Math::Float2(1.0f, 1.0f), Math::Float2(1.0f, 0.0f),
                    Math::Float2(1.0f,-1.0f), Math::Float2(1.0f, 1.0f),

                    Math::Float2(-1.0f, 1.0f), Math::Float2(0.0f, 0.0f),
                    Math::Float2(1.0f,-1.0f), Math::Float2(1.0f, 1.0f),
                    Math::Float2(-1.0f,-1.0f), Math::Float2(0.0f, 1.0f),
                };

                resultValue = video->createBuffer(&deferredVertexBuffer, sizeof(Math::Float4), 6, Video::BufferFlags::VertexBuffer | Video::BufferFlags::Static, vertexList);
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

            materialShaderMap[material] = shader;
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
            std::size_t hash = std::hashCombine(static_cast<UINT8>(renderStates.fillMode),
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
            std::size_t hash = std::hashCombine(depthStates.enable,
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
            std::size_t hash = std::hashCombine(blendStates.enable,
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
                std::hashCombine(hash, std::hashCombine(blendStates.targetStates[renderTarget].enable,
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

        STDMETHODIMP_(ResourceHandle) createRenderTarget(UINT32 width, UINT32 height, Video::Format format, UINT32 flags)
        {
            CComPtr<VideoTarget> renderTarget;
            video->createRenderTarget(&renderTarget, width, height, format, flags);
            return resourceManager.addUniqueResource(renderTarget);
        }

        STDMETHODIMP_(ResourceHandle) createDepthTarget(UINT32 width, UINT32 height, Video::Format format, UINT32 flags)
        {
            CComPtr<IUnknown> depthTarget;
            video->createDepthTarget(&depthTarget, width, height, format, flags);
            return resourceManager.addUniqueResource(depthTarget);
        }

        STDMETHODIMP_(ResourceHandle) createBuffer(LPCWSTR name, UINT32 stride, UINT32 count, DWORD flags, LPCVOID staticData)
        {
            std::size_t hash = std::hash<LPCWSTR>()(name);
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                CComPtr<VideoBuffer> buffer;
                resultValue = video->createBuffer(&buffer, stride, count, flags, staticData);
                if (SUCCEEDED(resultValue) && buffer)
                {
                    resultValue = buffer->QueryInterface(returnObject);
                }

                return resultValue;
            });
        }

        STDMETHODIMP_(ResourceHandle) createBuffer(LPCWSTR name, Video::Format format, UINT32 count, DWORD flags, LPCVOID staticData)
        {
            std::size_t hash = std::hash<LPCWSTR>()(name);
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                CComPtr<VideoBuffer> buffer;
                resultValue = video->createBuffer(&buffer, format, count, flags, staticData);
                if (SUCCEEDED(resultValue) && buffer)
                {
                    resultValue = buffer->QueryInterface(returnObject);
                }

                return resultValue;
            });
        }

        STDMETHODIMP_(ResourceHandle) loadTexture(LPCWSTR fileName, UINT32 flags)
        {
            std::size_t hash = std::hash<CStringW>()(CStringW(fileName).MakeReverse());
            return resourceManager.getResourceHandle(hash, [&](IUnknown **returnObject) -> HRESULT
            {
                REQUIRE_RETURN(fileName, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                if ((*fileName) && (*fileName) == L'*')
                {
                    if (_wcsnicmp(fileName, L"*color:", 7) == 0)
                    {
                        CComPtr<VideoTexture> texture;

                        UINT8 colorData[4] = { 0, 0, 0, 0 };
                        UINT32 colorPitch = 0;

                        float color1;
                        Math::Float2 color2;
                        Math::Float4 color4;
                        if (Evaluator::get((fileName + 7), color1))
                        {
                            resultValue = video->createTexture(&texture, 1, 1, 1, Video::Format::Byte, Video::TextureFlags::Resource);
                            colorData[0] = UINT8(color1 * 255.0f);
                            colorPitch = 1;
                        }
                        else if (Evaluator::get((fileName + 7), color2))
                        {
                            resultValue = video->createTexture(&texture, 1, 1, 1, Video::Format::Byte2, Video::TextureFlags::Resource);
                            colorData[0] = UINT8(color2.x * 255.0f);
                            colorData[1] = UINT8(color2.y * 255.0f);
                            colorPitch = 2;
                        }
                        else if (Evaluator::get((fileName + 7), color4))
                        {
                            resultValue = video->createTexture(&texture, 1, 1, 1, Video::Format::Byte4, Video::TextureFlags::Resource);
                            colorData[0] = UINT8(color4.x * 255.0f);
                            colorData[1] = UINT8(color4.y * 255.0f);
                            colorData[2] = UINT8(color4.z * 255.0f);
                            colorData[3] = UINT8(color4.w * 255.0f);
                            colorPitch = 4;
                        }

                        if (texture && colorPitch > 0)
                        {
                            video->updateTexture(texture, colorData, colorPitch);
                            resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
                        }
                    }
                    else if (_wcsnicmp(fileName, L"*normal:", 8) == 0)
                    {
                        CComPtr<VideoTexture> texture;
                        resultValue = video->createTexture(&texture, 1, 1, 1, Video::Format::Byte4, Video::TextureFlags::Resource);
                        if (texture)
                        {
                            Math::Float3 normal((String::to<Math::Float3>(fileName + 8) + 1.0f) * 0.5f);
                            UINT32 normalValue = UINT32(UINT8(normal.x * 255.0f)) |
                                UINT32(UINT8(normal.y * 255.0f) << 8) |
                                UINT32(UINT8(normal.z * 255.0f) << 16);
                            video->updateTexture(texture, &normalValue, 4);
                            resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
                        }
                    }
                }
                else
                {
                    CComPtr<VideoTexture> texture;
                    resultValue = video->loadTexture(&texture, String::format(L"%%root%%\\data\\textures\\%s", fileName), flags);
                    if (FAILED(resultValue))
                    {
                        resultValue = video->loadTexture(&texture, String::format(L"%%root%%\\data\\textures\\%s%s", fileName, L".dds"), flags);
                        if (FAILED(resultValue))
                        {
                            resultValue = video->loadTexture(&texture, String::format(L"%%root%%\\data\\textures\\%s%s", fileName, L".tga"), flags);
                            if (FAILED(resultValue))
                            {
                                resultValue = video->loadTexture(&texture, String::format(L"%%root%%\\data\\textures\\%s%s", fileName, L".png"), flags);
                                if (FAILED(resultValue))
                                {
                                    resultValue = video->loadTexture(&texture, String::format(L"%%root%%\\data\\textures\\%s%s", fileName, L".jpg"), flags);
                                }
                            }
                        }
                    }

                    if (SUCCEEDED(resultValue) && texture)
                    {
                        resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
                    }
                }

                return resultValue;
            });
        }

        STDMETHODIMP_(ProgramHandle) loadComputeProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            std::size_t hash = std::hashCombine(fileName, entryFunction);
            if (defineList)
            {
                for (auto &define : (*defineList))
                {
                    hash = std::hashCombine(hash, std::hashCombine(define.first, define.second));
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
            std::size_t hash = std::hashCombine(fileName, entryFunction);
            if (defineList)
            {
                for (auto &define : (*defineList))
                {
                    hash = std::hashCombine(hash, std::hashCombine(define.first, define.second));
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
            static VideoTarget *renderTargetList[8];
            for (UINT32 renderTarget = 0; renderTarget < renderTargetHandleCount; renderTarget++)
            {
                renderTargetList[renderTarget] = resourceManager.getResource<VideoTarget>(renderTargetHandleList[renderTarget]);
            }

            videoContext->setRenderTargets(renderTargetList, renderTargetHandleCount, resourceManager.getResource<IUnknown>(depthBuffer));
        }

        STDMETHODIMP_(void) setDefaultTargets(VideoContext *videoContext, ResourceHandle depthBuffer)
        {
            video->setDefaultTargets(videoContext, resourceManager.getResource<IUnknown>(depthBuffer));
        }

        // Render
        STDMETHODIMP_(void) queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(VideoContext *)> draw)
        {
            queuedDrawCalls[materialShaderMap[material]][plugin][material].push_back(draw);
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
            renderStateManager.clearResources();
            depthStateManager.clearResources();
            blendStateManager.clearResources();
        }

        STDMETHODIMP_(void) onUpdateEnd(float frameTime)
        {
            population->listEntities<TransformComponent, CameraComponent>([&](Entity *cameraEntity) -> void
            {
                auto &cameraTransform = cameraEntity->getComponent<TransformComponent>();
                Math::Float4x4 cameraMatrix(cameraTransform.rotation, cameraTransform.position);

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
                video->updateBuffer(this->cameraConstantBuffer, &cameraConstantData);

                VideoContext *videoContext = video->getDefaultContext();

                videoContext->geometryPipeline()->setConstantBuffer(this->cameraConstantBuffer, 0);
                videoContext->vertexPipeline()->setConstantBuffer(this->cameraConstantBuffer, 0);
                videoContext->pixelPipeline()->setConstantBuffer(this->cameraConstantBuffer, 0);
                videoContext->computePipeline()->setConstantBuffer(this->cameraConstantBuffer, 0);

                const Shape::Frustum viewFrustum(cameraConstantData.viewMatrix * cameraConstantData.projectionMatrix);

                concurrency::concurrent_vector<Light> concurrentVisibleLightList;
                population->listEntities<TransformComponent, PointLightComponent, ColorComponent>([&](Entity *lightEntity) -> void
                {
                    auto &lightTransformComponent = lightEntity->getComponent<TransformComponent>();
                    auto &pointLightComponent = lightEntity->getComponent<PointLightComponent>();
                    if (viewFrustum.isVisible(Shape::Sphere(lightTransformComponent.position, pointLightComponent.radius)))
                    {
                        auto &lightColorComponent = lightEntity->getComponent<ColorComponent>();

                        auto lightIterator = concurrentVisibleLightList.grow_by(1);
                        (*lightIterator).position = (cameraConstantData.viewMatrix * lightTransformComponent.position.w(1.0f)).xyz;
                        (*lightIterator).distance = (*lightIterator).position.getLengthSquared();
                        (*lightIterator).radius = pointLightComponent.radius;
                        (*lightIterator).color.set(lightColorComponent.value.xyz);
                    }
                });

                concurrency::parallel_sort(concurrentVisibleLightList.begin(), concurrentVisibleLightList.end(), [](const Light &leftLight, const Light &rightLight) -> bool
                {
                    return (leftLight.distance < rightLight.distance);
                });

                std::vector<Light> visibleLightList(concurrentVisibleLightList.begin(), concurrentVisibleLightList.end());
                concurrentVisibleLightList.clear();

                LPVOID lightingData = nullptr;
                if (SUCCEEDED(video->mapBuffer(lightingBuffer, &lightingData)))
                {
                    UINT32 passLightCount = std::min(visibleLightList.size(), MaxLightCount);
                    memcpy(lightingData, visibleLightList.data(), (sizeof(Light) * passLightCount));
                    video->unmapBuffer(lightingBuffer);

                    LightingConstantData lightingConstantData;
                    lightingConstantData.count = passLightCount;
                    video->updateBuffer(lightingConstantBuffer, &lightingConstantData);
                }

                queuedDrawCalls.clear();
                ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderScene, std::placeholders::_1, cameraEntity, &viewFrustum)));

                videoContext->pixelPipeline()->setSamplerStates(pointSamplerStates, 0);
                videoContext->pixelPipeline()->setSamplerStates(linearSamplerStates, 1);
                videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);
                for (auto &shaderPair : queuedDrawCalls)
                {
                    Shader *shader = shaderManager.getResource<Shader>(shaderPair.first);
                    shader->draw(videoContext,
                        [&](LPCVOID passData, bool lighting) -> void // drawForward
                    {
                        if (lighting)
                        {
                            videoContext->pixelPipeline()->setConstantBuffer(lightingConstantBuffer, 2);
                            videoContext->pixelPipeline()->setResource(lightingBuffer, 0);
                        }

                        for (auto &pluginPair : shaderPair.second)
                        {
                            Plugin *plugin = pluginManager.getResource<Plugin>(pluginPair.first);
                            plugin->enable(videoContext);

                            for (auto &materialPair : pluginPair.second)
                            {
                                Material *material = materialManager.getResource<Material>(materialPair.first);
                                shader->setResourceList(videoContext, material->getResourceList());

                                for (auto &onDraw : materialPair.second)
                                {
                                    onDraw(videoContext);
                                }
                            }
                        }
                    },
                        [&](LPCVOID passData, bool lighting) -> void // drawDeferred
                    {
                        if (lighting)
                        {
                            videoContext->pixelPipeline()->setConstantBuffer(lightingConstantBuffer, 2);
                            videoContext->pixelPipeline()->setResource(lightingBuffer, 0);
                        }

                        videoContext->setVertexBuffer(0, deferredVertexBuffer, 0);
                        videoContext->vertexPipeline()->setProgram(deferredVertexProgram);
                        videoContext->drawPrimitive(6, 0);
                    },
                        [&](LPCVOID passData, bool lighting, UINT32 dispatchWidth, UINT32 dispatchHeight, UINT32 dispatchDepth) -> void // drawCompute
                    {
                        if (lighting)
                        {
                            videoContext->computePipeline()->setConstantBuffer(lightingConstantBuffer, 2);
                            videoContext->computePipeline()->setResource(lightingBuffer, 0);
                        }

                        videoContext->dispatch(dispatchWidth, dispatchHeight, dispatchDepth);
                    });
                }
            });

            ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderOverlay, std::placeholders::_1)));

            video->present(true);
        }
    };

    REGISTER_CLASS(RenderImplementation)
}; // namespace Gek
