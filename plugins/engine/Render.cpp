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

        size_t remainder = hashCombine(ts...);   // not recursion!
        return hashCombine(seed, remainder);
    }
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
                struct CameraConstantBuffer
                {
                    Math::Float2 fieldOfView;
                    float minimumDistance;
                    float maximumDistance;
                    Math::Float4x4 viewMatrix;
                    Math::Float4x4 projectionMatrix;
                    Math::Float4x4 inverseProjectionMatrix;
                    Math::Float4x4 transformMatrix;
                };

                struct LightingConstantBuffer
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

                struct DrawCommand
                {
                    LPCVOID instanceData;
                    UINT32 instanceStride;
                    UINT32 instanceCount;
                    Handle vertexHandle;
                    UINT32 vertexCount;
                    UINT32 firstVertex;
                    Handle indexHandle;
                    UINT32 indexCount;
                    UINT32 firstIndex;

                    DrawCommand(Handle vertexHandle, UINT32 vertexCount, UINT32 firstVertex)
                        : vertexHandle(vertexHandle)
                        , vertexCount(vertexCount)
                        , firstVertex(firstVertex)
                    {
                    }

                    DrawCommand(Handle vertexHandle, UINT32 firstVertex, Handle indexHandle, UINT32 indexCount, UINT32 firstIndex)
                        : vertexHandle(vertexHandle)
                        , firstVertex(firstVertex)
                        , indexHandle(indexHandle)
                        , indexCount(indexCount)
                        , firstIndex(firstIndex)
                    {
                    }

                    DrawCommand(LPCVOID instanceData, UINT32 instanceStride, UINT32 instanceCount, Handle vertexHandle, UINT32 vertexCount, UINT32 firstVertex)
                        : instanceData(instanceData)
                        , instanceStride(instanceStride)
                        , instanceCount(instanceCount)
                        , vertexHandle(vertexHandle)
                        , vertexCount(vertexCount)
                        , firstVertex(firstVertex)
                    {
                    }

                    DrawCommand(LPCVOID instanceData, UINT32 instanceStride, UINT32 instanceCount, Handle vertexHandle, UINT32 firstVertex, Handle indexHandle, UINT32 indexCount, UINT32 firstIndex)
                        : instanceData(instanceData)
                        , instanceStride(instanceStride)
                        , instanceCount(instanceCount)
                        , vertexHandle(vertexHandle)
                        , firstVertex(firstVertex)
                        , indexHandle(indexHandle)
                        , indexCount(indexCount)
                        , firstIndex(firstIndex)
                    {
                    }
                };

            private:
                IUnknown *initializerContext;
                Video3D::Interface *video;
                Population::Interface *population;

                Handle nextResourceHandle;
                concurrency::concurrent_unordered_map<std::size_t, Handle> resourceMap;
                concurrency::concurrent_unordered_map<Handle, CComPtr<IUnknown>> resourceList;
                concurrency::concurrent_unordered_map<std::size_t, Handle> videoResourceMap;
                concurrency::concurrent_vector<Handle> videoResourceList;

                Handle cameraConstantBufferHandle;
                Handle lightingConstantBufferHandle;
                Handle lightingListHandle;
                Handle instanceBufferHandle;

                concurrency::concurrent_unordered_map<Handle, concurrency::concurrent_unordered_map<Handle, concurrency::concurrent_vector<DrawCommand>>> drawQueue;

            public:
                System(void)
                    : initializerContext(nullptr)
                    , video(nullptr)
                    , population(nullptr)
                    , nextResourceHandle(InvalidHandle)
                    , cameraConstantBufferHandle(InvalidHandle)
                    , lightingConstantBufferHandle(InvalidHandle)
                    , lightingListHandle(InvalidHandle)
                    , instanceBufferHandle(InvalidHandle)
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
                Handle loadResource(LPCWSTR fileName, REFCLSID className)
                {
                    REQUIRE_RETURN(initializerContext, E_FAIL);

                    Handle resourceHandle = InvalidHandle;

                    std::size_t hash = std::hash<LPCWSTR>()(fileName);
                    auto resourceMapIterator = resourceMap.find(hash);
                    if (resourceMapIterator != resourceMap.end())
                    {
                        resourceHandle = (*resourceMapIterator).second;
                    }
                    else
                    {
                        resourceMap[hash] = InvalidHandle;

                        CComPtr<CLASS> resource;
                        getContext()->createInstance(className, IID_PPV_ARGS(&resource));
                        if (resource && SUCCEEDED(resource->initialize(initializerContext, fileName)))
                        {
                            resourceHandle = InterlockedIncrement(&nextResourceHandle);
                            resourceList[resourceHandle] = resource;
                            resourceMap[hash] = resourceHandle;
                        }
                    }

                    return resourceHandle;
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
                        cameraConstantBufferHandle = video->createBuffer(sizeof(CameraConstantBuffer), 1, Video3D::BufferFlags::CONSTANT_BUFFER);
                        resultValue = (cameraConstantBufferHandle == InvalidHandle ? E_FAIL : S_OK);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        lightingConstantBufferHandle = video->createBuffer(sizeof(LightingConstantBuffer), 1, Video3D::BufferFlags::CONSTANT_BUFFER);
                        resultValue = (lightingConstantBufferHandle == InvalidHandle ? E_FAIL : S_OK);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        lightingListHandle = video->createBuffer(sizeof(Light), 256, Video3D::BufferFlags::DYNAMIC | Video3D::BufferFlags::STRUCTURED_BUFFER | Video3D::BufferFlags::RESOURCE);
                        resultValue = (lightingListHandle == InvalidHandle ? E_FAIL : S_OK);
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        instanceBufferHandle = video->createBuffer(sizeof(float), 1024, Video3D::BufferFlags::DYNAMIC | Video3D::BufferFlags::STRUCTURED_BUFFER | Video3D::BufferFlags::RESOURCE);
                        resultValue = (instanceBufferHandle == InvalidHandle ? E_FAIL : S_OK);
                    }

                    return resultValue;
                }

                STDMETHODIMP_(Handle) loadPlugin(LPCWSTR fileName)
                {
                    return loadResource<Plugin::Interface>(fileName, __uuidof(Plugin::Class));
                }

                STDMETHODIMP_(Handle) loadShader(LPCWSTR fileName)
                {
                    return loadResource<Shader::Interface>(fileName, __uuidof(Shader::Class));
                }

                STDMETHODIMP_(Handle) loadMaterial(LPCWSTR fileName)
                {
                    return loadResource<Material::Interface>(fileName, __uuidof(Material::Class));
                }

                STDMETHODIMP_(Handle) createRenderStates(const Video3D::RenderStates &renderStates)
                {
                    REQUIRE_RETURN(video, InvalidHandle);

                    Handle resourceHandle = InvalidHandle;
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
                    auto videoResourceIterator = videoResourceMap.find(hash);
                    if (videoResourceIterator != videoResourceMap.end())
                    {
                        resourceHandle = (*videoResourceIterator).second;
                    }
                    else
                    {
                        resourceHandle = video->createRenderStates(renderStates);
                        videoResourceMap.insert(std::make_pair(hash, resourceHandle));
                    }

                    return resourceHandle;
                }

                STDMETHODIMP_(Handle) createDepthStates(const Video3D::DepthStates &depthStates)
                {
                    REQUIRE_RETURN(video, InvalidHandle);

                    Handle resourceHandle = InvalidHandle;
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
                    auto videoResourceIterator = videoResourceMap.find(hash);
                    if (videoResourceIterator != videoResourceMap.end())
                    {
                        resourceHandle = (*videoResourceIterator).second;
                    }
                    else
                    {
                        resourceHandle = video->createDepthStates(depthStates);
                        videoResourceMap.insert(std::make_pair(hash, resourceHandle));
                    }

                    return resourceHandle;
                }

                STDMETHODIMP_(Handle) createBlendStates(const Video3D::UnifiedBlendStates &blendStates)
                {
                    REQUIRE_RETURN(video, InvalidHandle);

                    Handle resourceHandle = InvalidHandle;
                    std::size_t hash = std::hashCombine(blendStates.enable,
                        static_cast<UINT8>(blendStates.colorSource),
                        static_cast<UINT8>(blendStates.colorDestination),
                        static_cast<UINT8>(blendStates.colorOperation),
                        static_cast<UINT8>(blendStates.alphaSource),
                        static_cast<UINT8>(blendStates.alphaDestination),
                        static_cast<UINT8>(blendStates.alphaOperation),
                        blendStates.writeMask);
                    auto videoResourceIterator = videoResourceMap.find(hash);
                    if (videoResourceIterator != videoResourceMap.end())
                    {
                        resourceHandle = (*videoResourceIterator).second;
                    }
                    else
                    {
                        resourceHandle = video->createBlendStates(blendStates);
                        videoResourceMap.insert(std::make_pair(hash, resourceHandle));
                    }

                    return resourceHandle;
                }

                STDMETHODIMP_(Handle) createBlendStates(const Video3D::IndependentBlendStates &blendStates)
                {
                    REQUIRE_RETURN(video, InvalidHandle);

                    Handle resourceHandle = InvalidHandle;
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

                    auto videoResourceIterator = videoResourceMap.find(hash);
                    if (videoResourceIterator != videoResourceMap.end())
                    {
                        resourceHandle = (*videoResourceIterator).second;
                    }
                    else
                    {
                        resourceHandle = video->createBlendStates(blendStates);
                        videoResourceMap.insert(std::make_pair(hash, resourceHandle));
                    }

                    return resourceHandle;
                }

                STDMETHODIMP_(Handle) createRenderTarget(UINT32 width, UINT32 height, Video3D::Format format)
                {
                    REQUIRE_RETURN(video, InvalidHandle);
                    Handle resourceHandle = video->createRenderTarget(width, height, format);
                    videoResourceList.push_back(resourceHandle);
                    return resourceHandle;
                }

                STDMETHODIMP_(Handle) createDepthTarget(UINT32 width, UINT32 height, Video3D::Format format)
                {
                    REQUIRE_RETURN(video, InvalidHandle);
                    Handle resourceHandle = video->createDepthTarget(width, height, format);
                    videoResourceList.push_back(resourceHandle);
                    return resourceHandle;
                }

                STDMETHODIMP_(Handle) createBuffer(Video3D::Format format, UINT32 count, UINT32 flags, LPCVOID staticData)
                {
                    REQUIRE_RETURN(video, InvalidHandle);
                    Handle resourceHandle = video->createBuffer(format, count, flags, staticData);
                    videoResourceList.push_back(resourceHandle);
                    return resourceHandle;
                }

                STDMETHODIMP_(Handle) loadComputeProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
                {
                    REQUIRE_RETURN(video, InvalidHandle);

                    Handle resourceHandle = InvalidHandle;
                    std::size_t hash = std::hashCombine(fileName, entryFunction);
                    if (defineList)
                    {
                        for (auto &define : (*defineList))
                        {
                            hash = std::hashCombine(hash, std::hashCombine(define.first, define.second));
                        }
                    }

                    auto videoResourceIterator = videoResourceMap.find(hash);
                    if (videoResourceIterator != videoResourceMap.end())
                    {
                        resourceHandle = (*videoResourceIterator).second;
                    }
                    else
                    {
                        resourceHandle = video->loadComputeProgram(fileName, entryFunction, onInclude, defineList);
                        videoResourceMap.insert(std::make_pair(hash, resourceHandle));
                    }

                    return resourceHandle;
                }

                STDMETHODIMP_(Handle) loadPixelProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
                {
                    REQUIRE_RETURN(video, InvalidHandle);

                    Handle resourceHandle = InvalidHandle;
                    std::size_t hash = std::hashCombine(fileName, entryFunction);
                    if (defineList)
                    {
                        for (auto &define : (*defineList))
                        {
                            hash = std::hashCombine(hash, std::hashCombine(define.first, define.second));
                        }
                    }

                    auto videoResourceIterator = videoResourceMap.find(hash);
                    if (videoResourceIterator != videoResourceMap.end())
                    {
                        resourceHandle = (*videoResourceIterator).second;
                    }
                    else
                    {
                        resourceHandle = video->loadPixelProgram(fileName, entryFunction, onInclude, defineList);
                        videoResourceMap.insert(std::make_pair(hash, resourceHandle));
                    }

                    return resourceHandle;
                }

                STDMETHODIMP_(void) drawPrimitive(Handle pluginHandle, Handle materialHandle, Handle vertexHandle, UINT32 vertexCount, UINT32 firstVertex)
                {
                    drawQueue[pluginHandle][materialHandle].push_back(DrawCommand(vertexHandle, vertexCount, firstVertex));
                }

                STDMETHODIMP_(void) drawIndexedPrimitive(Handle pluginHandle, Handle materialHandle, Handle vertexHandle, UINT32 firstVertex, Handle indexHandle, UINT32 indexCount, UINT32 firstIndex)
                {
                    drawQueue[pluginHandle][materialHandle].push_back(DrawCommand(vertexHandle, firstVertex, indexHandle, indexCount, firstIndex));
                }

                STDMETHODIMP_(void) drawInstancedPrimitive(Handle pluginHandle, Handle materialHandle, LPCVOID instanceData, UINT32 instanceStride, UINT32 instanceCount, Handle vertexHandle, UINT32 vertexCount, UINT32 firstVertex)
                {
                    drawQueue[pluginHandle][materialHandle].push_back(DrawCommand(instanceData, instanceStride, instanceCount, vertexHandle, vertexCount, firstVertex));
                }

                STDMETHODIMP_(void) drawInstancedIndexedPrimitive(Handle pluginHandle, Handle materialHandle, LPCVOID instanceData, UINT32 instanceStride, UINT32 instanceCount, Handle vertexHandle, UINT32 firstVertex, Handle indexHandle, UINT32 indexCount, UINT32 firstIndex)
                {
                    drawQueue[pluginHandle][materialHandle].push_back(DrawCommand(instanceData, instanceStride, instanceCount, vertexHandle, firstVertex, indexHandle, indexCount, firstIndex));
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
                    REQUIRE_VOID_RETURN(video);

                    for (auto &videoResource : videoResourceList)
                    {
                        video->freeResource(videoResource);
                    }

                    for (auto &videoResource : videoResourceMap)
                    {
                        video->freeResource(videoResource.second);
                    }

                    resourceMap.clear();
                    resourceList.clear();
                    videoResourceMap.clear();
                    videoResourceList.clear();
                }

                STDMETHODIMP_(void) onUpdateEnd(float frameTime)
                {
                    if (frameTime > 0.0f)
                    {
                        video->clearDefaultRenderTarget(Math::Float4(0.0f, 1.0f, 0.0f, 1.0f));
                    }
                    else
                    {
                        video->clearDefaultRenderTarget(Math::Float4(1.0f, 0.0f, 0.0f, 1.0f));
                    }

                    population->listEntities({ Components::Transform::identifier, Components::Camera::identifier }, [&](Handle cameraHandle) -> void
                    {
                        auto &transformComponent = population->getComponent<Components::Transform::Data>(cameraHandle, Components::Transform::identifier);
                        auto &cameraComponent = population->getComponent<Components::Camera::Data>(cameraHandle, Components::Camera::identifier);
                        Math::Float4x4 cameraMatrix(transformComponent.rotation, transformComponent.position);

                        CameraConstantBuffer cameraConstantBuffer;
                        float displayAspectRatio = 1.0f;
                        float fieldOfView = Math::convertDegreesToRadians(cameraComponent.fieldOfView);
                        cameraConstantBuffer.fieldOfView.x = tan(fieldOfView * 0.5f);
                        cameraConstantBuffer.fieldOfView.y = (cameraConstantBuffer.fieldOfView.x / displayAspectRatio);
                        cameraConstantBuffer.minimumDistance = cameraComponent.minimumDistance;
                        cameraConstantBuffer.maximumDistance = cameraComponent.maximumDistance;
                        cameraConstantBuffer.viewMatrix = cameraMatrix.getInverse();
                        cameraConstantBuffer.projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, cameraComponent.minimumDistance, cameraComponent.maximumDistance);
                        cameraConstantBuffer.inverseProjectionMatrix = cameraConstantBuffer.projectionMatrix.getInverse();
                        cameraConstantBuffer.transformMatrix = (cameraConstantBuffer.viewMatrix * cameraConstantBuffer.projectionMatrix);
                        video->updateBuffer(cameraConstantBufferHandle, &cameraConstantBuffer);

                        Shape::Frustum viewFrustum(cameraConstantBuffer.transformMatrix, transformComponent.position);

                        concurrency::concurrent_vector<Light> concurrentVisibleLightList;
                        population->listEntities({ Components::Transform::identifier, Components::PointLight::identifier, Components::Color::identifier }, [&](Handle lightHandle) -> void
                        {
                            auto &transformComponent = population->getComponent<Components::Transform::Data>(lightHandle, Components::Transform::identifier);
                            auto &pointLightComponent = population->getComponent<Components::PointLight::Data>(lightHandle, Components::PointLight::identifier);
                            if (viewFrustum.isVisible(Shape::Sphere(transformComponent.position, pointLightComponent.radius)))
                            {
                                auto lightIterator = concurrentVisibleLightList.grow_by(1);
                                (*lightIterator).position = (cameraConstantBuffer.viewMatrix * Math::Float4(transformComponent.position, 1.0f));
                                (*lightIterator).distance = (*lightIterator).position.getLengthSquared();
                                (*lightIterator).range = pointLightComponent.radius;
                                (*lightIterator).color = population->getComponent<Components::Color::Data>(lightHandle, Components::Color::identifier);
                            }
                        }, true);

                        concurrency::parallel_sort(concurrentVisibleLightList.begin(), concurrentVisibleLightList.end(), [](const Light &leftLight, const Light &rightLight) -> bool
                        {
                            return (leftLight.distance < rightLight.distance);
                        });

                        std::vector<Light> visibleLightList(concurrentVisibleLightList.begin(), concurrentVisibleLightList.end());
                        concurrentVisibleLightList.clear();

                        drawQueue.clear();
                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::OnRenderScene, std::placeholders::_1, cameraHandle, viewFrustum)));
                        for (auto &pluginPair : drawQueue)
                        {
                            Handle pluginHandle = pluginPair.first;
                            for (auto &materialPair : pluginPair.second)
                            {
                                Handle materialHandle = materialPair.first;
                                for (auto &drawCommand : materialPair.second)
                                {
                                }
                            }
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
