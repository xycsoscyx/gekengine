#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include "GEK\Shape\OrientedBox.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\Utility\Common.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Engine\SystemInterface.h"
#include "GEK\Engine\ActionInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Size.h"
#include "GEK\Engine\Model.h"
#include <concurrent_unordered_map.h>
#include <memory>
#include <map>
#include <set>

namespace Gek
{
    namespace Engine
    {
        namespace Model
        {
            class System : public Context::BaseUser
                , public BaseObservable
                , public Engine::Population::Observer
                , public Engine::Render::Observer
                , public Engine::System::Interface
            {
            private:
                struct ModelData : public Shape::AlignedBox
                {
                };

            private:
                Engine::Render::Interface *render;
                Engine::Population::Interface *population;

                Handle programHandle;

                Handle nextModelHandle;
                concurrency::concurrent_unordered_map<Handle, ModelData> modelList;
                concurrency::concurrent_unordered_map<CStringW, Handle> modelNameList;
                concurrency::concurrent_unordered_map<Handle, Handle> entityModelList;

            public:
                System(void)
                    : render(nullptr)
                    , population(nullptr)
                    , programHandle(InvalidHandle)
                    , nextModelHandle(InvalidHandle)
                {
                }

                ~System(void)
                {
                    BaseObservable::removeObserver(population, getClass<Engine::Population::Observer>());
                }

                BEGIN_INTERFACE_LIST(System)
                    INTERFACE_LIST_ENTRY_COM(ObservableInterface)
                    INTERFACE_LIST_ENTRY_COM(Engine::Population::Observer)
                    INTERFACE_LIST_ENTRY_COM(Engine::Render::Observer)
                END_INTERFACE_LIST_USER

                Handle getModelHandle(LPCWSTR fileName)
                {
                    Handle modelHandle = InvalidHandle;
                    auto modelNameIterator = modelNameList.find(fileName);
                    if (modelNameIterator != modelNameList.end())
                    {
                        modelHandle = (*modelNameIterator).second;
                    }
                    else
                    {
                        modelHandle = InterlockedIncrement(&nextModelHandle);
                        modelNameList[fileName] = modelHandle;
                    }

                    return modelHandle;
                }

                // System::Interface
                STDMETHODIMP initialize(IUnknown *initializerContext)
                {
                    REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                    HRESULT resultValue = E_FAIL;
                    CComQIPtr<Engine::Render::Interface> render(initializerContext);
                    CComQIPtr<Engine::Population::Interface> population(initializerContext);
                    if (render && population)
                    {
                        this->render = render;
                        this->population = population;
                        resultValue = BaseObservable::addObserver(population, getClass<Engine::Population::Observer>());
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        programHandle = render->loadProgram(L"model");
                    }

                    return resultValue;
                };

                // Population::ObserverInterface
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
                    modelList.clear();
                    modelNameList.clear();
                    entityModelList.clear();
                }

                STDMETHODIMP_(void) onEntityCreated(Handle entityHandle)
                {
                    if (population->hasComponent(entityHandle, Engine::Components::Model::identifier) &&
                        population->hasComponent(entityHandle, Engine::Components::Transform::identifier))
                    {
                        auto &modelComponent = population->getComponent<Engine::Components::Model::Data>(entityHandle, Engine::Components::Model::identifier);
                        auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(entityHandle, Engine::Components::Transform::identifier);
                        entityModelList[entityHandle] = getModelHandle(modelComponent);
                    }
                }

                STDMETHODIMP_(void) onEntityDestroyed(Handle entityHandle)
                {
                    auto entityModelIterator = entityModelList.find(entityHandle);
                    if (entityModelIterator != entityModelList.end())
                    {
                        entityModelList.unsafe_erase(entityModelIterator);
                    }
                }

                STDMETHODIMP_(void) onUpdate(float frameTime)
                {
                }

                // Engine::Population::Observer
                STDMETHODIMP_(void) onRenderBegin(Handle viewerHandle)
                {
                }

                STDMETHODIMP_(void) onCullScene(Handle viewerHandle, const Gek::Shape::Frustum &viewFrustum)
                {
                    for (auto entity : entityModelList)
                    {
                        auto modelIterator = modelList.find(entity.second);
                        if (modelIterator != modelList.end())
                        {
                            auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(entity.first, Engine::Components::Transform::identifier);
                            Shape::OrientedBox orientedBox((*modelIterator).second, transformComponent.rotation, transformComponent.position);
                            if (viewFrustum.isVisible(orientedBox))
                            {
                            }
                        }
                    }
                }

                STDMETHODIMP_(void) onDrawScene(Handle viewerHandle, Gek::Video3D::ContextInterface *videoContext, UINT32 vertexAttributes)
                {
                }

                STDMETHODIMP_(void) onRenderEnd(Handle viewerHandle)
                {
                }

                STDMETHODIMP_(void) onRenderOverlay(void)
                {
                }
            };

            REGISTER_CLASS(System)
        }; // namespace Model
    }; // namespace Engine
}; // namespace Gek
