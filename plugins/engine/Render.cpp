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

                STDMETHODIMP_(void) enablePlugin(Handle pluginHandle)
                {
                    auto resourceIterator = resourceList.find(pluginHandle);
                    if (resourceIterator != resourceList.end())
                    {
                        IUnknown *resource = (*resourceIterator).second;
                    }
                }

                STDMETHODIMP_(Handle) loadShader(LPCWSTR fileName)
                {
                    return loadResource<Shader::Interface>(fileName, __uuidof(Shader::Class));
                }

                STDMETHODIMP_(void) enableShader(Handle shaderHandle)
                {
                    auto resourceIterator = resourceList.find(shaderHandle);
                    if (resourceIterator != resourceList.end())
                    {
                        IUnknown *resource = (*resourceIterator).second;
                    }
                }

                STDMETHODIMP_(Handle) loadMaterial(LPCWSTR fileName)
                {
                    return loadResource<Material::Interface>(fileName, __uuidof(Material::Class));
                }

                STDMETHODIMP_(void) enableMaterial(Handle materialHandle)
                {
                    auto resourceIterator = resourceList.find(materialHandle);
                    if (resourceIterator != resourceList.end())
                    {
                        IUnknown *resource = (*resourceIterator).second;
                    }
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
                        BaseObservable::sendEvent(Event<Render::Observer>(std::bind(&Render::Observer::onRenderBegin, std::placeholders::_1, cameraHandle)));

                        Shape::Frustum viewFrustum;
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
