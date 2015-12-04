#include "GEK\Engine\Render.h"
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
            return hash<LPCVOID>()((CLASS *)value);
        }
    };

    template <typename CLASS>
    struct hash<CComQIPtr<CLASS>>
    {
        size_t operator()(const CComQIPtr<CLASS> &value) const
        {
            return hash<LPCVOID>()((CLASS *)value);
        }
    };
};

namespace Gek
{
    static const UINT32 MaxLightCount = 500;

    class RenderImplementation : public ContextUserMixin
        , public ObservableMixin
        , public PopulationObserver
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

        struct DrawCommand
        {
            DrawType drawType;
            Plugin *plugin;
            Material *material;
            const std::vector<VideoBuffer *> &vertexBufferList;
            UINT32 instanceCount;
            UINT32 firstInstance;
            UINT32 vertexCount;
            UINT32 firstVertex;
            VideoBuffer* indexBuffer;
            UINT32 indexCount;
            UINT32 firstIndex;

            DrawCommand(IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 vertexCount, UINT32 firstVertex)
                : drawType(DrawType::DrawPrimitive)
                , plugin(dynamic_cast<Plugin *>(plugin))
                , material(dynamic_cast<Material *>(material))
                , vertexBufferList(vertexBufferList)
                , vertexCount(vertexCount)
                , firstVertex(firstVertex)
                , instanceCount(0)
                , firstInstance(0)
                , indexCount(0)
                , firstIndex(0)
            {
            }

            DrawCommand(IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 firstVertex, VideoBuffer *indexBuffer, UINT32 indexCount, UINT32 firstIndex)
                : drawType(DrawType::DrawIndexedPrimitive)
                , plugin(dynamic_cast<Plugin *>(plugin))
                , material(dynamic_cast<Material *>(material))
                , vertexBufferList(vertexBufferList)
                , firstVertex(firstVertex)
                , indexBuffer(indexBuffer)
                , indexCount(indexCount)
                , firstIndex(firstIndex)
                , instanceCount(0)
                , firstInstance(0)
                , vertexCount(0)
            {
            }

            DrawCommand(IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex)
                : drawType(DrawType::DrawInstancedPrimitive)
                , plugin(dynamic_cast<Plugin *>(plugin))
                , material(dynamic_cast<Material *>(material))
                , vertexBufferList(vertexBufferList)
                , instanceCount(instanceCount)
                , firstInstance(firstInstance)
                , vertexCount(vertexCount)
                , firstVertex(firstVertex)
                , indexCount(0)
                , firstIndex(0)
            {
            }

            DrawCommand(IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 instanceCount, UINT32 firstInstance, UINT32 firstVertex, VideoBuffer *indexBuffer, UINT32 indexCount, UINT32 firstIndex)
                : drawType(DrawType::DrawInstancedIndexedPrimitive)
                , plugin(dynamic_cast<Plugin *>(plugin))
                , material(dynamic_cast<Material *>(material))
                , vertexBufferList(vertexBufferList)
                , instanceCount(instanceCount)
                , firstInstance(firstInstance)
                , firstVertex(firstVertex)
                , indexBuffer(indexBuffer)
                , indexCount(indexCount)
                , firstIndex(firstIndex)
                , vertexCount(0)
            {
            }
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

        concurrency::concurrent_unordered_map<std::size_t, CComPtr<IUnknown>> resourceMap;
        concurrency::concurrent_unordered_map<Shader *, concurrency::concurrent_vector<DrawCommand>> drawQueue;

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
            INTERFACE_LIST_ENTRY_COM(Render)
        END_INTERFACE_LIST_USER

        template <typename CLASS>
        HRESULT getResource(CLASS **returnObject, std::size_t hash, std::function<HRESULT(CLASS **)> onResourceMissing)
        {
            HRESULT returnValue = E_FAIL;
            auto resourceIterator = resourceMap.find(hash);
            if (resourceIterator != resourceMap.end())
            {
                if ((*resourceIterator).second)
                {
                    returnValue = (*resourceIterator).second->QueryInterface(IID_PPV_ARGS(returnObject));
                }
            }
            else
            {
                resourceMap[hash] = nullptr;

                CComPtr<CLASS> resource;
                returnValue = onResourceMissing(&resource);
                if (SUCCEEDED(returnValue) && resource)
                {
                    returnValue = resource->QueryInterface(IID_PPV_ARGS(returnObject));
                    if (SUCCEEDED(returnValue))
                    {
                        resourceMap[hash] = resource;
                    }
                }
            }

            return returnValue;
        }

        // Render
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

        STDMETHODIMP loadPlugin(IUnknown **returnObject, LPCWSTR fileName)
        {
            REQUIRE_RETURN(initializerContext, E_FAIL);

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, std::hash<LPCWSTR>()(fileName), [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT returnValue = E_FAIL;

                CComPtr<Plugin> plugin;
                returnValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(PluginRegistration, &plugin));
                if (plugin)
                {
                    returnValue = plugin->initialize(initializerContext, fileName);
                    if (SUCCEEDED(returnValue))
                    {
                        returnValue = plugin->QueryInterface(returnObject);
                    }
                }

                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP loadShader(IUnknown **returnObject, LPCWSTR fileName)
        {
            REQUIRE_RETURN(initializerContext, E_FAIL);

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, std::hash<LPCWSTR>()(fileName), [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT returnValue = E_FAIL;
                CComPtr<Shader> shader;
                returnValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(ShaderRegistration, &shader));
                if (shader)
                {
                    returnValue = shader->initialize(initializerContext, fileName);
                    if (SUCCEEDED(returnValue))
                    {
                        returnValue = shader->QueryInterface(returnObject);
                    }
                }

                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP loadMaterial(IUnknown **returnObject, LPCWSTR fileName)
        {
            REQUIRE_RETURN(initializerContext, E_FAIL);

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, std::hash<LPCWSTR>()(fileName), [&](IUnknown **returnObject) -> HRESULT
            {
                HRESULT returnValue = E_FAIL;
                CComPtr<Material> material;
                returnValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(MaterialRegistration, &material));
                if (material)
                {
                    returnValue = material->initialize(initializerContext, fileName);
                    if (SUCCEEDED(returnValue))
                    {
                        returnValue = material->QueryInterface(returnObject);
                    }
                }

                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP createRenderStates(IUnknown **returnObject, const Video::RenderStates &renderStates)
        {
            REQUIRE_RETURN(video, E_FAIL);

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

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, hash, [&](IUnknown **returnObject)->HRESULT
            {
                HRESULT returnValue = E_FAIL;
                returnValue = video->createRenderStates(returnObject, renderStates);
                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP createDepthStates(IUnknown **returnObject, const Video::DepthStates &depthStates)
        {
            REQUIRE_RETURN(video, E_FAIL);

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

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, hash, [&](IUnknown **returnObject)->HRESULT
            {
                HRESULT returnValue = E_FAIL;
                returnValue = video->createDepthStates(returnObject, depthStates);
                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP createBlendStates(IUnknown **returnObject, const Video::UnifiedBlendStates &blendStates)
        {
            REQUIRE_RETURN(video, E_FAIL);

            std::size_t hash = std::hashCombine(blendStates.enable,
                static_cast<UINT8>(blendStates.colorSource),
                static_cast<UINT8>(blendStates.colorDestination),
                static_cast<UINT8>(blendStates.colorOperation),
                static_cast<UINT8>(blendStates.alphaSource),
                static_cast<UINT8>(blendStates.alphaDestination),
                static_cast<UINT8>(blendStates.alphaOperation),
                blendStates.writeMask);

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, hash, [&](IUnknown **returnObject)->HRESULT
            {
                HRESULT returnValue = E_FAIL;
                returnValue = video->createBlendStates(returnObject, blendStates);
                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP createBlendStates(IUnknown **returnObject, const Video::IndependentBlendStates &blendStates)
        {
            REQUIRE_RETURN(video, E_FAIL);

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

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, hash, [&](IUnknown **returnObject)->HRESULT
            {
                HRESULT returnValue = E_FAIL;
                returnValue = video->createBlendStates(returnObject, blendStates);
                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP createRenderTarget(VideoTexture **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags)
        {
            REQUIRE_RETURN(video, E_FAIL);

            HRESULT returnValue = E_FAIL;
            returnValue = video->createRenderTarget(returnObject, width, height, format, flags);
            return returnValue;
        }

        STDMETHODIMP createDepthTarget(IUnknown **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags)
        {
            REQUIRE_RETURN(video, E_FAIL);

            HRESULT returnValue = E_FAIL;
            returnValue = video->createDepthTarget(returnObject, width, height, format, flags);
            return returnValue;
        }

        STDMETHODIMP createBuffer(VideoBuffer **returnObject, LPCWSTR name, UINT32 stride, UINT32 count, DWORD flags, LPCVOID staticData)
        {
            REQUIRE_RETURN(video, E_FAIL);

            HRESULT returnValue = getResource<VideoBuffer>(returnObject, std::hash<LPCWSTR>()(name), [&](VideoBuffer **returnObject) -> HRESULT
            {
                HRESULT returnValue = E_FAIL;
                returnValue = video->createBuffer(returnObject, stride, count, flags, staticData);
                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP createBuffer(VideoBuffer **returnObject, LPCWSTR name, Video::Format format, UINT32 count, DWORD flags, LPCVOID staticData)
        {
            REQUIRE_RETURN(video, E_FAIL);

            HRESULT returnValue = getResource<VideoBuffer>(returnObject, std::hash<LPCWSTR>()(name), [&](VideoBuffer **returnObject) -> HRESULT
            {
                HRESULT returnValue = E_FAIL;
                returnValue = video->createBuffer(returnObject, format, count, flags, staticData);
                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP loadTexture(VideoTexture **returnObject, LPCWSTR fileName, UINT32 flags)
        {
            gekLogScope(fileName,
                flags);

            REQUIRE_RETURN(video, E_FAIL);

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<VideoTexture>(returnObject, std::hash<LPCWSTR>()(fileName), [&](VideoTexture **returnObject) -> HRESULT
            {
                HRESULT returnValue = E_FAIL;
                if (fileName)
                {
                    if ((*fileName) == L'*')
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
                                returnValue = video->createTexture(&texture, 1, 1, 1, Video::Format::Byte, Video::TextureFlags::Resource);
                                colorData[0] = UINT8(color1 * 255.0f);
                                colorPitch = 1;
                            }
                            else if (Evaluator::get((fileName + 7), color2))
                            {
                                returnValue = video->createTexture(&texture, 1, 1, 1, Video::Format::Byte2, Video::TextureFlags::Resource);
                                colorData[0] = UINT8(color2.x * 255.0f);
                                colorData[1] = UINT8(color2.y * 255.0f);
                                colorPitch = 2;
                            }
                            else if (Evaluator::get((fileName + 7), color4))
                            {
                                returnValue = video->createTexture(&texture, 1, 1, 1, Video::Format::Byte4, Video::TextureFlags::Resource);
                                colorData[0] = UINT8(color4.x * 255.0f);
                                colorData[1] = UINT8(color4.y * 255.0f);
                                colorData[2] = UINT8(color4.z * 255.0f);
                                colorData[3] = UINT8(color4.w * 255.0f);
                                colorPitch = 4;
                            }

                            if (texture && colorPitch > 0)
                            {
                                video->updateTexture(texture, colorData, colorPitch);
                                returnValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
                            }
                        }
                        else if (_wcsnicmp(fileName, L"*normal:", 8) == 0)
                        {
                            CComPtr<VideoTexture> texture;
                            returnValue = video->createTexture(&texture, 1, 1, 1, Video::Format::Byte4, Video::TextureFlags::Resource);
                            if (texture)
                            {
                                Math::Float3 normal((String::to<Math::Float3>(fileName + 8) + 1.0f) * 0.5f);
                                UINT32 normalValue = UINT32(UINT8(normal.x * 255.0f)) |
                                    UINT32(UINT8(normal.y * 255.0f) << 8) |
                                    UINT32(UINT8(normal.z * 255.0f) << 16);
                                video->updateTexture(texture, &normalValue, 4);
                                returnValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
                            }
                        }
                    }
                    else
                    {
                        returnValue = video->loadTexture(returnObject, String::format(L"%%root%%\\data\\textures\\%s", fileName), flags);
                        if (FAILED(returnValue))
                        {
                            returnValue = video->loadTexture(returnObject, String::format(L"%%root%%\\data\\textures\\%s%s", fileName, L".dds"), flags);
                            if (FAILED(returnValue))
                            {
                                returnValue = video->loadTexture(returnObject, String::format(L"%%root%%\\data\\textures\\%s%s", fileName, L".tga"), flags);
                                if (FAILED(returnValue))
                                {
                                    returnValue = video->loadTexture(returnObject, String::format(L"%%root%%\\data\\textures\\%s%s", fileName, L".png"), flags);
                                    if (FAILED(returnValue))
                                    {
                                        returnValue = video->loadTexture(returnObject, String::format(L"%%root%%\\data\\textures\\%s%s", fileName, L".jpg"), flags);
                                    }
                                }
                            }
                        }
                    }
                }

                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP loadComputeProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            REQUIRE_RETURN(video, E_FAIL);

            std::size_t hash = std::hashCombine(fileName, entryFunction);
            if (defineList)
            {
                for (auto &define : (*defineList))
                {
                    hash = std::hashCombine(hash, std::hashCombine(define.first, define.second));
                }
            }

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, hash, [&](IUnknown **returnObject)->HRESULT
            {
                HRESULT returnValue = E_FAIL;
                returnValue = video->loadComputeProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP loadPixelProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            REQUIRE_RETURN(video, E_FAIL);

            std::size_t hash = std::hashCombine(fileName, entryFunction);
            if (defineList)
            {
                for (auto &define : (*defineList))
                {
                    hash = std::hashCombine(hash, std::hashCombine(define.first, define.second));
                }
            }

            HRESULT returnValue = E_FAIL;
            returnValue = getResource<IUnknown>(returnObject, hash, [&](IUnknown **returnObject)->HRESULT
            {
                HRESULT returnValue = E_FAIL;
                returnValue = video->loadPixelProgram(returnObject, fileName, entryFunction, onInclude, defineList);
                return returnValue;
            });

            return returnValue;
        }

        STDMETHODIMP_(IUnknown *) findResource(LPCWSTR name)
        {
            REQUIRE_RETURN(name, nullptr);

            return nullptr;
        }

        STDMETHODIMP_(void) drawPrimitive(IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 vertexCount, UINT32 firstVertex)
        {
            REQUIRE_VOID_RETURN(material);
            REQUIRE_VOID_RETURN(plugin);

            drawQueue[dynamic_cast<Material *>(material)->getShader()].push_back(DrawCommand(plugin, material, vertexBufferList, vertexCount, firstVertex));
        }

        STDMETHODIMP_(void) drawIndexedPrimitive(IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 firstVertex, VideoBuffer *indexBuffer, UINT32 indexCount, UINT32 firstIndex)
        {
            REQUIRE_VOID_RETURN(material);
            REQUIRE_VOID_RETURN(plugin);

            drawQueue[dynamic_cast<Material *>(material)->getShader()].push_back(DrawCommand(plugin, material, vertexBufferList, firstVertex, indexBuffer, indexCount, firstIndex));
        }

        STDMETHODIMP_(void) drawInstancedPrimitive(IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex)
        {
            REQUIRE_VOID_RETURN(material);
            REQUIRE_VOID_RETURN(plugin);

            drawQueue[dynamic_cast<Material *>(material)->getShader()].push_back(DrawCommand(plugin, material, vertexBufferList, instanceCount, firstInstance, vertexCount, firstVertex));
        }

        STDMETHODIMP_(void) drawInstancedIndexedPrimitive(IUnknown *plugin, IUnknown *material, const std::vector<VideoBuffer *> &vertexBufferList, UINT32 instanceCount, UINT32 firstInstance, UINT32 firstVertex, VideoBuffer *indexBuffer, UINT32 indexCount, UINT32 firstIndex)
        {
            REQUIRE_VOID_RETURN(material);
            REQUIRE_VOID_RETURN(plugin);

            drawQueue[dynamic_cast<Material *>(material)->getShader()].push_back(DrawCommand(plugin, material, vertexBufferList, instanceCount, firstInstance, firstVertex, indexBuffer, indexCount, firstIndex));
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
            resourceMap.clear();
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
                for (auto &shaderPair : drawQueue)
                {
                    if (shaderPair.first == nullptr)
                    {
                        continue;
                    }

                    shaderPair.first->draw(defaultContext,
                        [&](LPCVOID passData, bool lighting) -> void // drawForward
                    {
                        if (lighting)
                        {
                            defaultContext->pixelPipeline()->setConstantBuffer(lightingConstantBuffer, 2);
                            defaultContext->pixelPipeline()->setResource(lightingBuffer, 0);
                        }

                        for (auto &drawCommand : shaderPair.second)
                        {
                            drawCommand.plugin->enable(defaultContext);
                            drawCommand.material->enable(defaultContext, passData);

                            UINT32 vertexBufferStage = 0;
                            for (auto &vertexBuffer : drawCommand.vertexBufferList)
                            {
                                defaultContext->setVertexBuffer(vertexBufferStage++, vertexBuffer, 0);
                            }

                            defaultContext->setIndexBuffer(drawCommand.indexBuffer, 0);
                            switch (drawCommand.drawType)
                            {
                            case DrawType::DrawPrimitive:
                                defaultContext->drawPrimitive(drawCommand.vertexCount, drawCommand.firstVertex);
                                break;

                            case DrawType::DrawIndexedPrimitive:
                                defaultContext->drawIndexedPrimitive(drawCommand.indexCount, drawCommand.firstIndex, drawCommand.firstVertex);
                                break;

                            case DrawType::DrawInstancedPrimitive:
                                defaultContext->drawInstancedPrimitive(drawCommand.instanceCount, drawCommand.firstInstance, drawCommand.vertexCount, drawCommand.firstVertex);
                                break;

                            case DrawType::DrawInstancedIndexedPrimitive:
                                defaultContext->drawInstancedIndexedPrimitive(drawCommand.instanceCount, drawCommand.firstInstance, drawCommand.indexCount, drawCommand.firstIndex, drawCommand.firstVertex);
                                break;
                            };
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
            });

            ObservableMixin::sendEvent(Event<RenderObserver>(std::bind(&RenderObserver::onRenderOverlay, std::placeholders::_1)));

            video->present(true);
        }
    };

    REGISTER_CLASS(RenderImplementation)
}; // namespace Gek
