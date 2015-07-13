#include "GEK\Engine\RenderInterface.h"
#include "GEK\Engine\PluginInterface.h"
#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Engine\MaterialInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
#include "GEK\Components\Light.h"
#include "GEK\Components\Color.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
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
    namespace Engine
    {
        namespace Render
        {
            class System : public Context::BaseUser
                , public BaseObservable
                , public Population::Observer
                , public Render::Interface
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
                    float distance;
                    float range;
                    Math::Float3 color;
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
                    std::vector<Video3D::BufferInterface *> vertexBufferList;
                    std::vector<UINT32> offsetList;
                    UINT32 instanceCount;
                    UINT32 vertexCount;
                    UINT32 firstVertex;
                    CComPtr<Video3D::BufferInterface> indexBuffer;
                    UINT32 indexCount;
                    UINT32 firstIndex;

                    DrawCommand(const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 vertexCount, UINT32 firstVertex)
                        : drawType(DrawType::DrawPrimitive)
                        , vertexBufferList(vertexBufferList)
                        , offsetList(vertexBufferList.size(), 0)
                        , vertexCount(vertexCount)
                        , firstVertex(firstVertex)
                        , instanceCount(0)
                        , indexCount(0)
                        , firstIndex(0)
                    {
                    }

                    DrawCommand(const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 firstVertex, Video3D::BufferInterface *indexBuffer, UINT32 indexCount, UINT32 firstIndex)
                        : drawType(DrawType::DrawIndexedPrimitive)
                        , vertexBufferList(vertexBufferList)
                        , offsetList(vertexBufferList.size(), 0)
                        , firstVertex(firstVertex)
                        , indexBuffer(indexBuffer)
                        , indexCount(indexCount)
                        , firstIndex(firstIndex)
                        , instanceCount(0)
                        , vertexCount(0)
                    {
                    }

                    DrawCommand(const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 instanceCount, UINT32 vertexCount, UINT32 firstVertex)
                        : drawType(DrawType::DrawInstancedPrimitive)
                        , vertexBufferList(vertexBufferList)
                        , offsetList(vertexBufferList.size(), 0)
                        , instanceCount(instanceCount)
                        , vertexCount(vertexCount)
                        , firstVertex(firstVertex)
                        , indexCount(0)
                        , firstIndex(0)
                    {
                    }

                    DrawCommand(const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 instanceCount, UINT32 firstVertex, Video3D::BufferInterface *indexBuffer, UINT32 indexCount, UINT32 firstIndex)
                        : drawType(DrawType::DrawInstancedIndexedPrimitive)
                        , vertexBufferList(vertexBufferList)
                        , offsetList(vertexBufferList.size(), 0)
                        , instanceCount(instanceCount)
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
                Video3D::Interface *video;
                Population::Interface *population;

                CComPtr<Video3D::BufferInterface> cameraConstantBuffer;
                CComPtr<Video3D::BufferInterface> lightingConstantBuffer;
                CComPtr<Video3D::BufferInterface> lightingBuffer;

                CComPtr<IUnknown> deferredVertexProgram;
                CComPtr<Video3D::BufferInterface> deferredVertexBuffer;

                concurrency::concurrent_unordered_map<std::size_t, CComPtr<IUnknown>> resourceMap;
                concurrency::concurrent_unordered_map<CComQIPtr<Plugin::Interface>, // plugin
                    concurrency::concurrent_unordered_map<CComQIPtr<Material::Interface>, // material
                    concurrency::concurrent_vector<DrawCommand>>> drawQueue; // command

            public:
                System(void)
                    : initializerContext(nullptr)
                    , video(nullptr)
                    , population(nullptr)
                {
                }

                ~System(void)
                {
                    BaseObservable::removeObserver(population, getClass<Engine::Population::Observer>());
                }

                BEGIN_INTERFACE_LIST(System)
                    INTERFACE_LIST_ENTRY_COM(Gek::ObservableInterface)
                    INTERFACE_LIST_ENTRY_COM(Population::Observer)
                    INTERFACE_LIST_ENTRY_COM(Interface)
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

                // Render::Interface
                STDMETHODIMP initialize(IUnknown *initializerContext)
                {
                    REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                    HRESULT resultValue = E_FAIL;
                    CComQIPtr<Video3D::Interface> video(initializerContext);
                    CComQIPtr<Population::Interface> population(initializerContext);
                    if (video && population)
                    {
                        this->video = video;
                        this->population = population;
                        this->initializerContext = initializerContext;
                        resultValue = BaseObservable::addObserver(population, getClass<Engine::Population::Observer>());
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = video->createBuffer(&cameraConstantBuffer, sizeof(CameraConstantData), 1, Video3D::BufferFlags::ConstantBuffer);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = video->createBuffer(&lightingConstantBuffer, sizeof(LightingConstantData), 1, Video3D::BufferFlags::ConstantBuffer);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = video->createBuffer(&lightingBuffer, sizeof(Light), 256, Video3D::BufferFlags::Dynamic | Video3D::BufferFlags::StructuredBuffer | Video3D::BufferFlags::Resource);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        static const char program[] =
                            "struct Vertex                                                                      \r\n" \
                            "{                                                                                  \r\n" \
                            "    float2 position : POSITION;                                                    \r\n" \
                            "    float2 texcoord : TEXCOORD0;                                                   \r\n" \
                            "};                                                                                 \r\n" \
                            "                                                                                   \r\n" \
                            "struct Pixel                                                                       \r\n" \
                            "{                                                                                  \r\n" \
                            "    float4 position : SV_POSITION;                                                 \r\n" \
                            "    float2 texcoord : TEXCOORD0;                                                   \r\n" \
                            "};                                                                                 \r\n" \
                            "                                                                                   \r\n" \
                            "Pixel mainVertexProgram(in Vertex vertex)                                          \r\n" \
                            "{                                                                                  \r\n" \
                            "    Pixel pixel;                                                                   \r\n" \
                            "    pixel.position = float4(vertex.position, 0.0f, 1.0f);                          \r\n" \
                            "    pixel.texcoord = vertex.texcoord;                                              \r\n" \
                            "    return pixel;                                                                  \r\n" \
                            "}                                                                                  \r\n" \
                            "                                                                                   \r\n";

                        static const std::vector<Video3D::InputElement> elementList =
                        {
                            Video3D::InputElement(Video3D::Format::Float2, "POSITION", 0),
                            Video3D::InputElement(Video3D::Format::Float2, "TEXCOORD", 0),
                        };

                        resultValue = video->compileVertexProgram(&deferredVertexProgram, program, "mainVertexProgram", elementList);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        static const Math::Float2 vertexList[] =
                        {
                            Math::Float2(-1.0f, -1.0f), Math::Float2(0.0f, 0.0f),
                            Math::Float2( 1.0f, -1.0f), Math::Float2(1.0f, 0.0f),
                            Math::Float2( 1.0f,  1.0f), Math::Float2(1.0f, 1.0f),

                            Math::Float2(-1.0f, -1.0f), Math::Float2(0.0f, 0.0f),
                            Math::Float2( 1.0f,  1.0f), Math::Float2(1.0f, 1.0f),
                            Math::Float2(-1.0f,  1.0f), Math::Float2(0.0f, 1.0f),
                        };

                        resultValue = video->createBuffer(&deferredVertexBuffer, sizeof(Math::Float4), 6, Video3D::BufferFlags::VertexBuffer | Video3D::BufferFlags::Static, vertexList);
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
                        CComPtr<Plugin::Interface> plugin;
                        returnValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(Plugin::Class, &plugin));
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
                        CComPtr<Shader::Interface> shader;
                        returnValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(Shader::Class, &shader));
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
                        CComPtr<Material::Interface> material;
                        returnValue = getContext()->createInstance(CLSID_IID_PPV_ARGS(Material::Class, &material));
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

                STDMETHODIMP createRenderStates(IUnknown **returnObject, const Video3D::RenderStates &renderStates)
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

                STDMETHODIMP createDepthStates(IUnknown **returnObject, const Video3D::DepthStates &depthStates)
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

                STDMETHODIMP createBlendStates(IUnknown **returnObject, const Video3D::UnifiedBlendStates &blendStates)
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

                STDMETHODIMP createBlendStates(IUnknown **returnObject, const Video3D::IndependentBlendStates &blendStates)
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

                STDMETHODIMP createRenderTarget(Video3D::TextureInterface **returnObject, UINT32 width, UINT32 height, Video3D::Format format)
                {
                    REQUIRE_RETURN(video, E_FAIL);

                    HRESULT returnValue = E_FAIL;
                    returnValue = video->createRenderTarget(returnObject, width, height, format);
                    return returnValue;
                }

                STDMETHODIMP createDepthTarget(IUnknown **returnObject, UINT32 width, UINT32 height, Video3D::Format format)
                {
                    REQUIRE_RETURN(video, E_FAIL);

                    HRESULT returnValue = E_FAIL;
                    returnValue = video->createDepthTarget(returnObject, width, height, format);
                    return returnValue;
                }

                STDMETHODIMP createBuffer(Video3D::BufferInterface **returnObject, Video3D::Format format, UINT32 count, UINT32 flags, LPCVOID staticData)
                {
                    REQUIRE_RETURN(video, E_FAIL);

                    HRESULT returnValue = E_FAIL;
                    returnValue = video->createBuffer(returnObject, format, count, flags, staticData);
                    return returnValue;
                }

                STDMETHODIMP loadTexture(Video3D::TextureInterface **returnObject, LPCWSTR fileName)
                {
                    REQUIRE_RETURN(video, E_FAIL);

                    HRESULT returnValue = E_FAIL;
                    returnValue = getResource<Video3D::TextureInterface>(returnObject, std::hash<LPCWSTR>()(fileName), [&](Video3D::TextureInterface **returnObject) -> HRESULT
                    {
                        HRESULT returnValue = E_FAIL;
                        if (fileName)
                        {
                            if ((*fileName) == L'*')
                            {
                            }
                            else
                            {
                                returnValue = video->loadTexture(returnObject, String::format(L"%%root%%\\data\\textures\\%s", fileName), 0);
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

                STDMETHODIMP_(void) drawPrimitive(IUnknown *plugin, IUnknown *material, const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 vertexCount, UINT32 firstVertex)
                {
                    drawQueue[plugin][material].push_back(DrawCommand(vertexBufferList, vertexCount, firstVertex));
                }

                STDMETHODIMP_(void) drawIndexedPrimitive(IUnknown *plugin, IUnknown *material, const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 firstVertex, Video3D::BufferInterface *indexBuffer, UINT32 indexCount, UINT32 firstIndex)
                {
                    drawQueue[plugin][material].push_back(DrawCommand(vertexBufferList, firstVertex, indexBuffer, indexCount, firstIndex));
                }

                STDMETHODIMP_(void) drawInstancedPrimitive(IUnknown *plugin, IUnknown *material, const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 instanceCount, UINT32 vertexCount, UINT32 firstVertex)
                {
                    drawQueue[plugin][material].push_back(DrawCommand(vertexBufferList, instanceCount, vertexCount, firstVertex));
                }

                STDMETHODIMP_(void) drawInstancedIndexedPrimitive(IUnknown *plugin, IUnknown *material, const std::vector<Video3D::BufferInterface *> &vertexBufferList, UINT32 instanceCount,  UINT32 firstVertex, Video3D::BufferInterface *indexBuffer, UINT32 indexCount, UINT32 firstIndex)
                {
                    drawQueue[plugin][material].push_back(DrawCommand(vertexBufferList, instanceCount, firstVertex, indexBuffer, indexCount, firstIndex));
                }

                // Population::Observer
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
                    population->listEntities({ Components::Transform::identifier, Components::Camera::identifier }, [&](const Population::Entity &cameraEntity) -> void
                    {
                        auto &transformComponent = population->getComponent<Components::Transform::Data>(cameraEntity, Components::Transform::identifier);
                        auto &cameraComponent = population->getComponent<Components::Camera::Data>(cameraEntity, Components::Camera::identifier);
                        Math::Float4x4 cameraMatrix(transformComponent.rotation, transformComponent.position);

                        CameraConstantData cameraConstantData;
                        float displayAspectRatio = 1.0f;
                        float fieldOfView = Math::convertDegreesToRadians(cameraComponent.fieldOfView);
                        cameraConstantData.fieldOfView.x = tan(fieldOfView * 0.5f);
                        cameraConstantData.fieldOfView.y = (cameraConstantData.fieldOfView.x / displayAspectRatio);
                        cameraConstantData.minimumDistance = cameraComponent.minimumDistance;
                        cameraConstantData.maximumDistance = cameraComponent.maximumDistance;
                        cameraConstantData.viewMatrix = cameraMatrix.getInverse();
                        cameraConstantData.projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, cameraComponent.minimumDistance, cameraComponent.maximumDistance);
                        cameraConstantData.inverseProjectionMatrix = cameraConstantData.projectionMatrix.getInverse();
                        video->updateBuffer(this->cameraConstantBuffer, &cameraConstantData);

                        video->getDefaultContext()->getGeometrySystem()->setConstantBuffer(this->cameraConstantBuffer, 0);
                        video->getDefaultContext()->getVertexSystem()->setConstantBuffer(this->cameraConstantBuffer, 0);
                        video->getDefaultContext()->getPixelSystem()->setConstantBuffer(this->cameraConstantBuffer, 0);
                        video->getDefaultContext()->getComputeSystem()->setConstantBuffer(this->cameraConstantBuffer, 0);

                        const Shape::Frustum viewFrustum(cameraConstantData.viewMatrix * cameraConstantData.projectionMatrix);

                        concurrency::concurrent_vector<Light> concurrentVisibleLightList;
                        population->listEntities({ Components::Transform::identifier, Components::PointLight::identifier, Components::Color::identifier }, [&](const Population::Entity &lightEntity) -> void
                        {
                            auto &transformComponent = population->getComponent<Components::Transform::Data>(lightEntity, Components::Transform::identifier);
                            auto &pointLightComponent = population->getComponent<Components::PointLight::Data>(lightEntity, Components::PointLight::identifier);
                            if (viewFrustum.isVisible(Shape::Sphere(transformComponent.position, pointLightComponent.radius)))
                            {
                                auto lightIterator = concurrentVisibleLightList.grow_by(1);
                                (*lightIterator).position = (cameraConstantData.viewMatrix * Math::Float4(transformComponent.position, 1.0f));
                                (*lightIterator).distance = (*lightIterator).position.getLengthSquared();
                                (*lightIterator).range = pointLightComponent.radius;
                                (*lightIterator).color = population->getComponent<Components::Color::Data>(lightEntity, Components::Color::identifier);
                            }
                        }, true);

                        concurrency::parallel_sort(concurrentVisibleLightList.begin(), concurrentVisibleLightList.end(), [](const Light &leftLight, const Light &rightLight) -> bool
                        {
                            return (leftLight.distance < rightLight.distance);
                        });

                        std::vector<Light> visibleLightList(concurrentVisibleLightList.begin(), concurrentVisibleLightList.end());
                        concurrentVisibleLightList.clear();
                        
                        LPVOID lightingData = nullptr;
                        if (SUCCEEDED(video->mapBuffer(lightingBuffer, &lightingData)))
                        {
                            UINT32 lightCount = std::min(visibleLightList.size(), size_t(256));
                            memcpy(lightingData, visibleLightList.data(), (sizeof(Light) * lightCount));
                            video->unmapBuffer(lightingBuffer);

                            LightingConstantData lightingConstantData;
                            lightingConstantData.count = lightCount;
                            video->updateBuffer(lightingConstantBuffer, &lightingConstantData);
                        }

                        drawQueue.clear();
                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::OnRenderScene, std::placeholders::_1, cameraEntity, viewFrustum)));

                        concurrency::concurrent_unordered_map<CComQIPtr<Shader::Interface>, // shader
                            concurrency::concurrent_unordered_map<Plugin::Interface *, // plugin
                            concurrency::concurrent_unordered_map<Material::Interface *, // material
                            concurrency::concurrent_vector<DrawCommand> *>>> sortedDrawQueue; // command
                        for (auto &pluginPair : drawQueue)
                        {
                            for (auto &materialPair : pluginPair.second)
                            {
                                CComPtr<IUnknown> shader;
                                materialPair.first->getShader(&shader);
                                if (shader)
                                {
                                    sortedDrawQueue[(IUnknown *)shader][pluginPair.first][materialPair.first] = &materialPair.second;
                                }
                            }
                        }

                        video->getDefaultContext()->setPrimitiveType(Video3D::PrimitiveType::TriangleList);
                        for (auto &shaderPair : sortedDrawQueue)
                        {
                            shaderPair.first->draw(video->getDefaultContext(),
                                [&](LPCVOID passData, bool lighting) -> void // drawForward
                            {
                                if (lighting)
                                {
                                    video->getDefaultContext()->getPixelSystem()->setConstantBuffer(lightingConstantBuffer, 2);
                                    video->getDefaultContext()->getPixelSystem()->setResource(lightingBuffer, 0);
                                }

                                for (auto &pluginPair : shaderPair.second)
                                {
                                    pluginPair.first->enable(video->getDefaultContext());
                                    for (auto &materialPair : pluginPair.second)
                                    {
                                        materialPair.first->enable(video->getDefaultContext(), passData);
                                        for (auto &drawCommand : (*materialPair.second))
                                        {
                                            video->getDefaultContext()->setIndexBuffer(drawCommand.indexBuffer, 0);
                                            video->getDefaultContext()->setVertexBufferList(0, drawCommand.vertexBufferList, drawCommand.offsetList);
                                            switch (drawCommand.drawType)
                                            {
                                            case DrawType::DrawPrimitive:
                                                video->getDefaultContext()->drawPrimitive(drawCommand.vertexCount, drawCommand.firstVertex);
                                                break;

                                            case DrawType::DrawIndexedPrimitive:
                                                video->getDefaultContext()->drawIndexedPrimitive(drawCommand.indexCount, drawCommand.firstIndex, drawCommand.firstVertex);
                                                break;

                                            case DrawType::DrawInstancedPrimitive:
                                                video->getDefaultContext()->drawInstancedPrimitive(drawCommand.instanceCount, 0, drawCommand.vertexCount, drawCommand.firstVertex);
                                                break;

                                            case DrawType::DrawInstancedIndexedPrimitive:
                                                video->getDefaultContext()->drawInstancedIndexedPrimitive(drawCommand.instanceCount, 0, drawCommand.indexCount, drawCommand.firstIndex, drawCommand.firstVertex);
                                                break;
                                            };
                                        }
                                    }
                                }
                            },
                                [&](LPCVOID passData, bool lighting) -> void // drawDeferred
                            {
                                if (lighting)
                                {
                                    video->getDefaultContext()->getPixelSystem()->setConstantBuffer(lightingConstantBuffer, 2);
                                    video->getDefaultContext()->getPixelSystem()->setResource(lightingBuffer, 0);
                                }

                                video->getDefaultContext()->setVertexBuffer(0, deferredVertexBuffer, 0);
                                video->getDefaultContext()->getVertexSystem()->setProgram(deferredVertexProgram);
                                video->getDefaultContext()->drawPrimitive(6, 0);
                            },
                                [&](LPCVOID passData, bool lighting) -> void // drawCompute
                            {
                                if (lighting)
                                {
                                    video->getDefaultContext()->getComputeSystem()->setConstantBuffer(lightingConstantBuffer, 2);
                                    video->getDefaultContext()->getComputeSystem()->setResource(lightingBuffer, 0);
                                }
                            });
                        }
                    });

                    BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onRenderOverlay, std::placeholders::_1)));

                    video->present(true);
                }
            };

            REGISTER_CLASS(System)
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
