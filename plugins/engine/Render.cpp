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

    template <class HANDLE, typename... ARGS>
    class ResourceManager
    {
    private:
        std::function<HRESULT(IUnknown **, ARGS... args)> loadResource;

        UINT64 nextIdentifier;
        concurrency::concurrent_unordered_map<std::size_t, HANDLE> hashMap;
        concurrency::concurrent_unordered_map<HANDLE, CComPtr<IUnknown>> resourceMap;

    public:
        ResourceManager(std::function<HRESULT(IUnknown **, ARGS... args)> loadResource)
            : loadResource(loadResource)
            , nextIdentifier(0)
        {
        }

        ~ResourceManager(void)
        {
        }

        BEGIN_INTERFACE_LIST(ResourceManager)
        END_INTERFACE_LIST_USER

        void clearResources(void)
        {
            hashMap.clear();
            resourceMap.clear();
        }

        HANDLE getResourceHandle(std::size_t hash, ARGS... args)
        {
            HANDLE handle;
            auto hashIterator = hashMap.find(hash);
            if (hashIterator == hashMap.end())
            {
                handle = HANDLE(InterlockedIncrement(&nextIdentifier));
                hashMap[hash] = handle;
                resourceMap[handle] = nullptr;

                CComPtr<IUnknown> resource;
                if (SUCCEEDED(loadResource(&resource, args...)) && resource)
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
                handle = HANDLE(InterlockedIncrement(&nextIdentifier));
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

        enum class DrawType : UINT8
        {
            DrawPrimitive = 0,
            DrawIndexedPrimitive,
            DrawInstancedPrimitive,
            DrawInstancedIndexedPrimitive,
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

        ResourceManager<ProgramHandle, bool, LPCWSTR, LPCSTR, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)>, std::unordered_map<CStringA, CStringA> *> programManager;
        ResourceManager<PluginHandle, LPCWSTR> pluginManager;
        ResourceManager<MaterialHandle, LPCWSTR> materialManager;
        ResourceManager<ShaderHandle, LPCWSTR> shaderManager;
        ResourceManager<TextureHandle, LPCWSTR, UINT32> textureManager;
        ResourceManager<BufferHandle, UINT32, Video::Format, UINT32, DWORD, LPCVOID> bufferManager;
        ResourceManager<RenderStatesHandle, const Video::RenderStates &> renderStateManager;
        ResourceManager<DepthStatesHandle, const Video::DepthStates &> depthStateManager;
        ResourceManager<BlendStatesHandle, const Video::UnifiedBlendStates *, const Video::IndependentBlendStates *> blendStateManager;

        //STDMETHOD(createRenderTarget)                   (THIS_ VideoTexture **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        //STDMETHOD(createDepthTarget)                    (THIS_ IUnknown **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;

    public:
        RenderImplementation(void)
            : initializerContext(nullptr)
            , video(nullptr)
            , population(nullptr)
            , pluginManager([&](IUnknown **returnObject, LPCWSTR fileName) -> HRESULT
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
                })
            , materialManager([&](IUnknown **returnObject, LPCWSTR fileName) -> HRESULT
                {
                    HRESULT resultValue = E_FAIL;

                    CComPtr<Material> material;
                    resultValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(MaterialRegistration, &material));
                    if (material)
                    {
                        resultValue = material->initialize(initializerContext, fileName);
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = material->QueryInterface(returnObject);
                        }
                    }

                    return resultValue;
                })
            , shaderManager([&](IUnknown **returnObject, LPCWSTR fileName) -> HRESULT
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
                })
            , textureManager([&](IUnknown **returnObject, LPCWSTR fileName, UINT32 flags) -> HRESULT
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
                })
            , bufferManager([&](IUnknown **returnObject, UINT32 stride, Video::Format format, UINT32 count, DWORD flags, LPCVOID staticData) -> HRESULT
                {
                    HRESULT resultValue = E_FAIL;
                    CComPtr<VideoBuffer> buffer;
                    if (stride > 0)
                    {
                        resultValue = video->createBuffer(&buffer, stride, count, flags, staticData);
                    }
                    else if(format != Video::Format::Invalid)
                    {
                        resultValue = video->createBuffer(&buffer, format, count, flags, staticData);
                    }

                    if (SUCCEEDED(resultValue) && buffer)
                    {
                        resultValue = buffer->QueryInterface(returnObject);
                    }

                    return resultValue;
                })
            , renderStateManager([&](IUnknown **returnObject, const Video::RenderStates &renderStates) -> HRESULT
                {
                    HRESULT resultValue = E_FAIL;
                    resultValue = video->createRenderStates(returnObject, renderStates);
                    return resultValue;
                })
            , depthStateManager([&](IUnknown **returnObject, const Video::DepthStates &depthStates) -> HRESULT
                {
                    HRESULT resultValue = E_FAIL;
                    resultValue = video->createDepthStates(returnObject, depthStates);
                    return resultValue;
                })
            , blendStateManager([&](IUnknown **returnObject, const Video::UnifiedBlendStates *unifiedBlendStates, const Video::IndependentBlendStates *independentBlendStates) -> HRESULT
                {
                    HRESULT resultValue = E_FAIL;
                    if (unifiedBlendStates)
                    {
                        resultValue = video->createBlendStates(returnObject, *unifiedBlendStates);
                    }
                    else if(independentBlendStates)
                    {
                        resultValue = video->createBlendStates(returnObject, *independentBlendStates);
                    }

                    return resultValue;
                })
            , programManager([&](IUnknown **returnObject, bool computeProgram, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList) -> HRESULT
                {
                    HRESULT resultValue = E_FAIL;
                    if (computeProgram)
                    {
                        resultValue = video->loadComputeProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                    }
                    else
                    {
                        resultValue = video->loadPixelProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                    }

                    return resultValue;
                })
        {
        }

        ~RenderImplementation(void)
        {
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(RenderImplementation)
            INTERFACE_LIST_ENTRY_COM(Observable)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
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

        // Render
        STDMETHODIMP_(void) queueDrawCall(std::size_t plugin, std::size_t material, std::function<void(VideoContext *)> draw)
        {
        }

        // Resources
        STDMETHODIMP_(PluginHandle) loadPlugin(LPCWSTR fileName)
        {
            std::size_t hash = std::hash<LPCWSTR>()(fileName);
            return pluginManager.getResourceHandle(hash, fileName);
        }

        STDMETHODIMP_(MaterialHandle) loadMaterial(LPCWSTR fileName)
        {
            std::size_t hash = std::hash<LPCWSTR>()(fileName);
            return materialManager.getResourceHandle(hash, fileName);
        }

        STDMETHODIMP_(ShaderHandle) loadShader(LPCWSTR fileName)
        {
            std::size_t hash = std::hash<LPCWSTR>()(fileName);
            return shaderManager.getResourceHandle(hash, fileName);
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
            return renderStateManager.getResourceHandle(hash, renderStates);
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
            return depthStateManager.getResourceHandle(hash, depthStates);
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
            return blendStateManager.getResourceHandle(hash, &blendStates, nullptr);
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

            return blendStateManager.getResourceHandle(hash, nullptr, &blendStates);
        }

        STDMETHODIMP_(TextureHandle) createRenderTarget(UINT32 width, UINT32 height, Video::Format format, UINT32 flags)
        {
            CComPtr<VideoTexture> renderTarget;
            video->createRenderTarget(&renderTarget, width, height, format, flags);
            return textureManager.addUniqueResource(renderTarget);
        }

        STDMETHODIMP_(TextureHandle) createDepthTarget(UINT32 width, UINT32 height, Video::Format format, UINT32 flags)
        {
            CComPtr<IUnknown> depthTarget;
            video->createDepthTarget(&depthTarget, width, height, format, flags);
            return textureManager.addUniqueResource(depthTarget);
        }

        STDMETHODIMP_(BufferHandle) createBuffer(LPCWSTR name, UINT32 stride, UINT32 count, DWORD flags, LPCVOID staticData)
        {
            std::size_t hash = std::hash<LPCWSTR>()(name);
            return bufferManager.getResourceHandle(hash, stride, Video::Format::Invalid, count, flags, staticData);
        }

        STDMETHODIMP_(BufferHandle) createBuffer(LPCWSTR name, Video::Format format, UINT32 count, DWORD flags, LPCVOID staticData)
        {
            std::size_t hash = std::hash<LPCWSTR>()(name);
            return bufferManager.getResourceHandle(hash, 0, format, count, flags, staticData);
        }

        STDMETHODIMP_(TextureHandle) loadTexture(LPCWSTR fileName, UINT32 flags)
        {
            std::size_t hash = std::hash<LPCWSTR>()(fileName);
            return textureManager.getResourceHandle(hash, fileName, flags);
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

            return programManager.getResourceHandle(hash, true, fileName, entryFunction, onInclude, defineList);
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

            return programManager.getResourceHandle(hash, false, fileName, entryFunction, onInclude, defineList);
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

        STDMETHODIMP_(void) setResource(VideoPipeline *videoPipeline, TextureHandle textureHandle, UINT32 stage)
        {
            videoPipeline->setResource(textureManager.getResource<IUnknown>(textureHandle), stage);
        }

        STDMETHODIMP_(void) setResource(VideoPipeline *videoPipeline, BufferHandle bufferHandle, UINT32 stage)
        {
            videoPipeline->setResource(bufferManager.getResource<IUnknown>(bufferHandle), stage);
        }

        STDMETHODIMP_(void) setProgram(VideoPipeline *videoPipeline, ProgramHandle programHandle)
        {
            videoPipeline->setProgram(programManager.getResource<IUnknown>(programHandle));
        }

        STDMETHODIMP_(void) setVertexBuffer(VideoContext *videoContext, UINT32 slot, BufferHandle bufferHandle, UINT32 offset)
        {
            videoContext->setVertexBuffer(slot, bufferManager.getResource<VideoBuffer>(bufferHandle), offset);
        }

        STDMETHODIMP_(void) setIndexBuffer(VideoContext *videoContext, BufferHandle bufferHandle, UINT32 offset)
        {
            videoContext->setIndexBuffer(bufferManager.getResource<VideoBuffer>(bufferHandle), offset);
        }

        STDMETHODIMP_(void) clearRenderTarget(VideoContext *videoContext, TextureHandle textureHandle, const Math::Float4 &color)
        {
            videoContext->clearRenderTarget(textureManager.getResource<VideoTexture>(textureHandle), color);
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
            textureManager.clearResources();
            bufferManager.clearResources();
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

                VideoContext *defaultContext = video->getDefaultContext();

                defaultContext->geometryPipeline()->setConstantBuffer(this->cameraConstantBuffer, 0);
                defaultContext->vertexPipeline()->setConstantBuffer(this->cameraConstantBuffer, 0);
                defaultContext->pixelPipeline()->setConstantBuffer(this->cameraConstantBuffer, 0);
                defaultContext->computePipeline()->setConstantBuffer(this->cameraConstantBuffer, 0);

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

                ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::OnRenderScene, std::placeholders::_1, cameraEntity, &viewFrustum)));

                defaultContext->pixelPipeline()->setSamplerStates(pointSamplerStates, 0);
                defaultContext->pixelPipeline()->setSamplerStates(linearSamplerStates, 1);
                defaultContext->setPrimitiveType(Video::PrimitiveType::TriangleList);
/*
                for (auto &shaderPair : drawQueue)
                {
                    shaderPair.first->draw(defaultContext,
                        [&](LPCVOID passData, bool lighting) -> void // drawForward
                    {
                        if (lighting)
                        {
                            defaultContext->pixelPipeline()->setConstantBuffer(lightingConstantBuffer, 2);
                            defaultContext->pixelPipeline()->setResource(lightingBuffer, 0);
                        }

                        for (auto &pluginPair : shaderPair.second)
                        {
                            Plugin *plugin = getResource<Plugin>(pluginPair.first);
                            if (plugin)
                            {
                                plugin->enable(defaultContext);
                                for (auto &materialPair : pluginPair.second)
                                {
                                    Material *material = getResource<Material>(materialPair.first);
                                    if (material)
                                    {
                                        material->enable(defaultContext, passData);
                                        for (auto &drawCall : materialPair.second)
                                        {
                                            drawCall(defaultContext);
                                        }
                                    }
                                }
                            }
                        }
                    },
                        [&](LPCVOID passData, bool lighting) -> void // drawDeferred
                    {
                        if (lighting)
                        {
                            defaultContext->pixelPipeline()->setConstantBuffer(lightingConstantBuffer, 2);
                            defaultContext->pixelPipeline()->setResource(lightingBuffer, 0);
                        }

                        defaultContext->setVertexBuffer(0, deferredVertexBuffer, 0);
                        defaultContext->vertexPipeline()->setProgram(deferredVertexProgram);
                        defaultContext->drawPrimitive(6, 0);
                    },
                        [&](LPCVOID passData, bool lighting, UINT32 dispatchWidth, UINT32 dispatchHeight, UINT32 dispatchDepth) -> void // drawCompute
                    {
                        if (lighting)
                        {
                            defaultContext->computePipeline()->setConstantBuffer(lightingConstantBuffer, 2);
                            defaultContext->computePipeline()->setResource(lightingBuffer, 0);
                        }

                        defaultContext->dispatch(dispatchWidth, dispatchHeight, dispatchDepth);
                    });

                    shaderPair.second.clear();
                }
*/
            });

            ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderOverlay, std::placeholders::_1)));

            video->present(true);
        }
    };

    REGISTER_CLASS(RenderImplementation)
}; // namespace Gek
