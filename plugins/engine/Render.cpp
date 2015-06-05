#include "GEK\Engine\RenderInterface.h"
#include "GEK\Engine\PluginInterface.h"
#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Engine\MaterialInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
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
                struct GlobalConstantBuffer
                {
                    Math::Float2 fieldOfView;
                    float minimumDistance;
                    float maximumDistance;
                    Math::Float4x4 viewMatrix;
                    Math::Float4x4 projectionMatrix;
                    Math::Float4x4 inverseProjectionMatrix;
                    Math::Float4x4 transformMatrix;
                };

            private:
                IUnknown *initializerContext;
                Video3D::Interface *video;
                Population::Interface *population;

                Handle nextResourceHandle;
                concurrency::concurrent_unordered_map<std::size_t, Handle> resourceHashList;
                concurrency::concurrent_unordered_map<Handle, CComPtr<IUnknown>> resourceList;

            public:
                System(void)
                    : initializerContext(nullptr)
                    , video(nullptr)
                    , population(nullptr)
                    , nextResourceHandle(InvalidHandle)
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

                        GlobalConstantBuffer globalConstantBuffer;
                        float displayAspectRatio = 1.0f;
                        float fieldOfView = Math::convertDegreesToRadians(cameraComponent.fieldOfView);
                        globalConstantBuffer.fieldOfView.x = tan(fieldOfView * 0.5f);
                        globalConstantBuffer.fieldOfView.y = (globalConstantBuffer.fieldOfView.x / displayAspectRatio);
                        globalConstantBuffer.minimumDistance = cameraComponent.minimumDistance;
                        globalConstantBuffer.maximumDistance = cameraComponent.maximumDistance;
                        globalConstantBuffer.viewMatrix = cameraMatrix.getInverse();
                        globalConstantBuffer.projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, cameraComponent.minimumDistance, cameraComponent.maximumDistance);
                        globalConstantBuffer.inverseProjectionMatrix = globalConstantBuffer.projectionMatrix.getInverse();
                        globalConstantBuffer.transformMatrix = (globalConstantBuffer.viewMatrix * globalConstantBuffer.projectionMatrix);

                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onRenderBegin, std::placeholders::_1, cameraHandle)));

                        Shape::Frustum viewFrustum(globalConstantBuffer.transformMatrix, transformComponent.position);
                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onCullScene, std::placeholders::_1, cameraHandle, viewFrustum)));

                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onDrawScene, std::placeholders::_1, cameraHandle, video->getDefaultContext(), 0xFFFFFFFF)));

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
