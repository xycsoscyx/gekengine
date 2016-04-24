﻿#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Utility\ShuntingYard.h"
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
static std::uniform_real_distribution<float> zeroToOne(0.0f, 1.0f);
static std::uniform_real_distribution<float> negativeOneToOne(-1.0f, 1.0f);

namespace Gek
{
    class ParticlesProcessorImplementation : public ContextUserMixin
        , public PopulationObserver
        , public RenderObserver
        , public Processor
    {
    public:
        __declspec(align(16))
        struct ParticleData
        {
            Math::Float3 position;
            Math::Float3 velocity;
            float life;
            float style;

            ParticleData(void)
                : life(0.0f)
                , style(0.0f)
            {
            }
        };

        struct EmitterData : public Shapes::AlignedBox
        {
            MaterialHandle material;
            ResourceHandle colorMap;
            ResourceHandle sizeMap;
            std::uniform_real_distribution<float> lifeExpectancy;
            std::vector<ParticleData> particles;
        };

    private:
        PluginResources *resources;
        Render *render;
        Population *population;
        UINT32 updateHandle;

        PluginHandle plugin;
        ResourceHandle particleBuffer;

        ShuntingYard shuntingYard;
        typedef std::unordered_map<Entity *, EmitterData> DataEntityMap;
        DataEntityMap entityDataList;

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
                particleBuffer = resources->createBuffer(nullptr, sizeof(ParticleData), 100, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
                if (!particleBuffer)
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
                auto &particlesComponent = entity->getComponent<ParticlesComponent>();

                auto &emitter = entityDataList[entity];
                emitter.particles.resize(particlesComponent.density);
                emitter.material = resources->loadMaterial(L"Particles\\" + particlesComponent.material);
                emitter.colorMap = resources->loadTexture(L"Particles\\" + particlesComponent.colorMap, nullptr, 0);
                emitter.sizeMap = resources->loadTexture(L"Particles\\" + particlesComponent.sizeMap, nullptr, 0);
                emitter.lifeExpectancy = std::uniform_real_distribution<float>(particlesComponent.lifeExpectancy.x, particlesComponent.lifeExpectancy.y);
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

        struct minimum : public concurrency::combinable<float>
        {
            minimum(float defaultValue)
                : concurrency::combinable<float>([&] {return defaultValue; })
            {
            }

            void set(float value)
            {
                auto &localValue = local();
                localValue = std::min(value, localValue);
            }

            float get(void)
            {
                return combine([](float left, float right) { return std::min(left, right); });
            }
        };

        struct maximum : public concurrency::combinable<float>
        {
            maximum(float defaultValue)
                : concurrency::combinable<float>([&] {return defaultValue; })
            {
            }

            void set(float value)
            {
                auto &localValue = local();
                localValue = std::max(value, localValue);
            }

            float get(void)
            {
                return combine([](float left, float right) { return std::max(left, right); });
            }
        };

        STDMETHODIMP_(void) onUpdate(bool isIdle)
        {
            if (!isIdle)
            {
                float frameTime = population->getFrameTime();
                concurrency::parallel_for_each(entityDataList.begin(), entityDataList.end(), [&](DataEntityMap::value_type &dataEntity) -> void
                {
                    Entity *entity = dataEntity.first;
                    EmitterData &emitter = const_cast<EmitterData &>(dataEntity.second);
                    auto &transformComponent = entity->getComponent<TransformComponent>();

                    minimum minimum[3] = { (Math::Infinity), (Math::Infinity), (Math::Infinity) };
                    maximum maximum[3] = { (-Math::Infinity), (-Math::Infinity), (-Math::Infinity) };
                    concurrency::parallel_for_each(emitter.particles.begin(), emitter.particles.end(), [&](auto &particle) -> void
                    {
                        particle.life -= frameTime / emitter.lifeExpectancy.max();
                        if (particle.life <= 0.0f)
                        {
                            particle.life = emitter.lifeExpectancy(mersineTwister) / emitter.lifeExpectancy.max();
                            particle.style = zeroToOne(mersineTwister);
                            particle.position = transformComponent.position;
                            particle.velocity.x = negativeOneToOne(mersineTwister);
                            particle.velocity.y = zeroToOne(mersineTwister);
                            particle.velocity.z = negativeOneToOne(mersineTwister);
                            particle.velocity.normalize();
                        }

                        particle.velocity += (Math::Float3(0.0f, -32.174f, 0.0f) * frameTime * 0.05f);
                        particle.position += (particle.velocity * frameTime);
                        
                        for (auto index : { 0, 1, 2 })
                        {
                            minimum[index].set(particle.position[index]);
                            maximum[index].set(particle.position[index]);
                        }
                    });

                    for (auto index : { 0, 1, 2 })
                    {
                        emitter.minimum[index] = minimum[index].get();
                        emitter.maximum[index] = maximum[index].get();
                    }
                });
            }
        }

        // RenderObserver
        static void drawCall(RenderContext *renderContext, PluginResources *resources, Entity *entity, const EmitterData &emitter, ResourceHandle particleBuffer)
        {
            ParticleData *particleData = nullptr;
            if (SUCCEEDED(resources->mapBuffer(particleBuffer, (LPVOID *)&particleData)))
            {
                memcpy(particleData, emitter.particles.data(), (sizeof(ParticleData) * emitter.particles.size()));
                resources->unmapBuffer(particleBuffer);

                resources->setResource(renderContext->vertexPipeline(), particleBuffer, 0);
                resources->setResource(renderContext->vertexPipeline(), emitter.colorMap, 1);
                resources->setResource(renderContext->vertexPipeline(), emitter.sizeMap, 2);
                renderContext->getContext()->drawPrimitive((emitter.particles.size() * 6), 0);
            }
        }

        STDMETHODIMP_(void) onRenderScene(Entity *cameraEntity, const Shapes::Frustum *viewFrustum)
        {
            GEK_REQUIRE_VOID_RETURN(cameraEntity);
            GEK_REQUIRE_VOID_RETURN(viewFrustum);

            concurrency::parallel_for_each(entityDataList.begin(), entityDataList.end(), [&](auto &dataEntity) -> void
            {
                Entity *entity = dataEntity.first;
                const EmitterData &emitter = dataEntity.second;
                if (viewFrustum->isVisible(emitter))
                {
                    render->queueDrawCall(plugin, emitter.material, std::bind(drawCall, std::placeholders::_1, resources, entity, emitter, particleBuffer));
                }
            });
        }
    };

    REGISTER_CLASS(ParticlesProcessorImplementation)
}; // namespace Gek

