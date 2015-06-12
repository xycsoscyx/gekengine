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
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Shape\Sphere.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

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

            private:
                IUnknown *initializerContext;
                Video3D::Interface *video;
                Population::Interface *population;

                Handle nextResourceHandle;
                concurrency::concurrent_unordered_map<std::size_t, Handle> resourceHashList;
                concurrency::concurrent_unordered_map<Handle, CComPtr<IUnknown>> resourceList;

                Handle cameraConstantBufferHandle;
                Handle lightingConstantBufferHandle;
                Handle lightingListHandle;

            public:
                System(void)
                    : initializerContext(nullptr)
                    , video(nullptr)
                    , population(nullptr)
                    , nextResourceHandle(InvalidHandle)
                    , cameraConstantBufferHandle(InvalidHandle)
                    , lightingConstantBufferHandle(InvalidHandle)
                    , lightingListHandle(InvalidHandle)
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
                    auto resourceHashIterator = resourceHashList.find(hash);
                    if (resourceHashIterator != resourceHashList.end())
                    {
                        resourceHandle = (*resourceHashIterator).second;
                    }
                    else
                    {
                        resourceHashList[hash] = InvalidHandle;

                        CComPtr<CLASS> resource;
                        getContext()->createInstance(className, IID_PPV_ARGS(&resource));
                        if (resource && SUCCEEDED(resource->initialize(initializerContext, fileName)))
                        {
                            resourceHandle = InterlockedIncrement(&nextResourceHandle);
                            resourceList[resourceHandle] = resource;
                            resourceHashList[hash] = resourceHandle;
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

                STDMETHODIMP_(void) drawPrimitive(Handle pluginHandle, Handle materialHandle, Handle vertexHandle, Handle indexHandle, UINT32 vertexCount, UINT32 firstVertex)
                {
                }

                STDMETHODIMP_(void) drawIndexedPrimitive(Handle pluginHandle, Handle materialHandle, Handle vertexHandle, Handle indexHandle, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
                {
                }

                STDMETHODIMP_(void) drawInstancedPrimitive(Handle pluginHandle, Handle materialHandle, Handle vertexHandle, Handle indexHandle, LPCVOID instanceData, UINT32 instanceStride, UINT32 instanceCount, UINT32 vertexCount, UINT32 firstVertex)
                {
                }

                STDMETHODIMP_(void) drawInstancedIndexedPrimitive(Handle pluginHandle, Handle materialHandle, Handle vertexHandle, Handle indexHandle, LPCVOID instanceData, UINT32 instanceStride, UINT32 instanceCount, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
                {
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
                    resourceHashList.clear();
                    resourceList.clear();
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

                        concurrency::parallel_sort(concurrentVisibleLightList.begin(), concurrentVisibleLightList.end(), [transformComponent](const Light &leftLight, const Light &rightLight) -> bool
                        {
                            return (leftLight.distance < rightLight.distance);
                        });

                        std::vector<Light> visibleLightList(concurrentVisibleLightList.begin(), concurrentVisibleLightList.end());
                        concurrentVisibleLightList.clear();

                        for (UINT32 index = 0; index < visibleLightList.size(); index += 256)
                        {
                            Light *lightingListBuffer = nullptr;
                            if (SUCCEEDED(video->mapBuffer(lightingListHandle, (LPVOID *)&lightingListBuffer)))
                            {
                                UINT32 instanceCount = std::min(256U, (visibleLightList.size() - index));
                                memcpy(lightingListBuffer, visibleLightList.data(), (sizeof(Light) * instanceCount));
                                video->unmapBuffer(lightingListHandle);
                            }
                        }

                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onRenderBegin, std::placeholders::_1, cameraHandle)));

                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onCullScene, std::placeholders::_1, cameraHandle, viewFrustum)));

                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onRenderEnd, std::placeholders::_1, cameraHandle)));
                    });

                    BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onRenderOverlay, std::placeholders::_1)));

                    video->present(true);
                }
            };

            REGISTER_CLASS(System)
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
