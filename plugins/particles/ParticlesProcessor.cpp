#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Context\COM.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Color.h"
#include "GEK\Engine\Particles.h"
#include <concurrent_queue.h>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <ppl.h>
#include <functional>
#include <random>
#include <map>

static std::random_device randomDevice;
static std::mt19937 mersineTwister(randomDevice());
static std::uniform_real_distribution<float> uniformRealDistribution(-1.0f, 1.0f);

namespace Gek
{
    class ParticlesProcessorImplementation : public ContextUserMixin
        , public PopulationObserver
        , public RenderObserver
        , public Processor
    {
    public:
        struct ConstantData
        {
            Math::Float4x4 transform;
        };

        struct InstanceData
        {
            Math::Float3 position;
            float rotation;
            Math::Color color;
            Math::Float2 size;
            float buffer[2];
        };

        struct ParticleData
        {
        };

        struct EmitterData
        {
            MaterialHandle material;
            std::vector<ParticleData> particles;
        };

    private:
        PluginResources *resources;
        Render *render;
        Population *population;
        UINT32 updateHandle;

        PluginHandle plugin;
        ResourceHandle constantBuffer;
        ResourceHandle instanceBuffer;

        std::unordered_map<Entity *, EmitterData> entityDataList;

    public:
        ParticlesProcessorImplementation(void)
            : resources(nullptr)
            , render(nullptr)
            , population(nullptr)
            , updateHandle(0)
        {
        }

        ~ParticlesProcessorImplementation(void)
        {
            population->removeUpdatePriority(updateHandle);
            ObservableMixin::removeObserver(render, getClass<RenderObserver>());
            ObservableMixin::removeObserver(population, getClass<PopulationObserver>());
        }

        BEGIN_INTERFACE_LIST(ParticlesProcessorImplementation)
            INTERFACE_LIST_ENTRY_COM(PopulationObserver)
            INTERFACE_LIST_ENTRY_COM(RenderObserver)
            INTERFACE_LIST_ENTRY_COM(Processor)
        END_INTERFACE_LIST_USER

        // System::Interface
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            GEK_REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<PluginResources> resources(initializerContext);
            CComQIPtr<Render> render(initializerContext);
            CComQIPtr<Population> population(initializerContext);
            if (resources && render && population)
            {
                this->resources = resources;
                this->render = render;
                this->population = population;
                resultValue = ObservableMixin::addObserver(population, getClass<PopulationObserver>());
                if (SUCCEEDED(resultValue))
                {
                    updateHandle = population->setUpdatePriority(this, 60);
                }
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = ObservableMixin::addObserver(render, getClass<RenderObserver>());
            }

            if (SUCCEEDED(resultValue))
            {
                plugin = resources->loadPlugin(L"particles");
                if (!plugin)
                {
                    resultValue = E_FAIL;
                }
            }

            if (SUCCEEDED(resultValue))
            {
                constantBuffer = resources->createBuffer(nullptr, sizeof(ConstantData), 1, Video::BufferType::Constant, Video::BufferFlags::Mappable);
                if (!constantBuffer)
                {
                    resultValue = E_FAIL;
                }
            }

            if (SUCCEEDED(resultValue))
            {
                instanceBuffer = resources->createBuffer(nullptr, sizeof(InstanceData), 100, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
                if (!instanceBuffer)
                {
                    resultValue = E_FAIL;
                }
            }

            return resultValue;
        };

        // PopulationObserver
        STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
        {
            if (FAILED(resultValue))
            {
                onFree();
            }
        }

        STDMETHODIMP_(void) onFree(void)
        {
            entityDataList.clear();
        }

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            GEK_REQUIRE_VOID_RETURN(resources);
            GEK_REQUIRE_VOID_RETURN(entity);

            if (entity->hasComponents<ParticlesComponent, TransformComponent>())
            {
                auto &emitter = entityDataList[entity];
                emitter.particles.resize(100);
                emitter.material = resources->loadMaterial(L"silver");
            }
        }

        STDMETHODIMP_(void) onEntityDestroyed(Entity *entity)
        {
            GEK_REQUIRE_VOID_RETURN(entity);

            auto dataEntityIterator = entityDataList.find(entity);
            if (dataEntityIterator != entityDataList.end())
            {
                entityDataList.erase(dataEntityIterator);
            }
        }

        STDMETHODIMP_(void) onUpdate(bool isIdle)
        {
            if (!isIdle)
            {
                std::for_each(entityDataList.begin(), entityDataList.end(), [&](const std::pair<Entity *, EmitterData> &dataEntity) -> void
                {
                    Entity *entity = dataEntity.first;
                    const EmitterData &emitter = dataEntity.second;
                    for (auto &particle : emitter.particles)
                    {
                    }
                });
            }
        }

        // RenderObserver
        STDMETHODIMP_(void) onRenderScene(Entity *cameraEntity, const Shapes::Frustum *viewFrustum)
        {
            GEK_REQUIRE_VOID_RETURN(cameraEntity);
            GEK_REQUIRE_VOID_RETURN(viewFrustum);

            std::for_each(entityDataList.begin(), entityDataList.end(), [&](const std::pair<Entity *, EmitterData> &dataEntity) -> void
            {
                Entity *entity = dataEntity.first;
                const EmitterData &emitter = dataEntity.second;
                static auto drawCall = [](RenderContext *renderContext, PluginResources *resources, Entity *entity, const EmitterData &emitter, ResourceHandle constantBuffer, ResourceHandle instanceBuffer) -> void
                {
                    LPVOID constantData = nullptr;
                    if (SUCCEEDED(resources->mapBuffer(constantBuffer, &constantData)))
                    {
                        const auto &transformComponent = entity->getComponent<TransformComponent>();
                        (*(Math::Float4x4 *)constantData) = transformComponent.getMatrix();
                        resources->unmapBuffer(constantBuffer);

                        InstanceData *instanceData = nullptr;
                        if (SUCCEEDED(resources->mapBuffer(instanceBuffer, (LPVOID *)&instanceData)))
                        {
                            for (auto &particle : emitter.particles)
                            {
                                auto &instance = (*instanceData++);
                                instance.position.x = uniformRealDistribution(mersineTwister);
                                instance.position.y = uniformRealDistribution(mersineTwister);
                                instance.position.z = uniformRealDistribution(mersineTwister);
                                instance.rotation = uniformRealDistribution(mersineTwister);
                                instance.size = uniformRealDistribution(mersineTwister);
                                instance.color.r = uniformRealDistribution(mersineTwister);
                                instance.color.g = uniformRealDistribution(mersineTwister);
                                instance.color.b = uniformRealDistribution(mersineTwister);
                                instance.color.a = 1.0f;
                            }

                            resources->unmapBuffer(instanceBuffer);

                            resources->setConstantBuffer(renderContext->vertexPipeline(), constantBuffer, 4);
                            resources->setResource(renderContext->vertexPipeline(), instanceBuffer, 0);
                            renderContext->getContext()->drawPrimitive((emitter.particles.size() * 4), 0);
                        }
                    }
                };

                render->queueDrawCall(plugin, emitter.material, std::bind(drawCall, std::placeholders::_1, resources, entity, emitter, constantBuffer, instanceBuffer));
            });
        }
    };

    REGISTER_CLASS(ParticlesProcessorImplementation)
}; // namespace Gek

