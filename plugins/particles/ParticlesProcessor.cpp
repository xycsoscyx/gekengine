#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Utility\ShuntingYard.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Engine.h"
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
static std::uniform_real_distribution<float> halfToOne(0.5f, 1.0f);
static std::uniform_real_distribution<float> negativeOneToOne(-1.0f, 1.0f);

namespace Gek
{
    static const uint32_t ParticleBufferCount = 1000;

    class ParticlesProcessorImplementation
        : public ContextRegistration<ParticlesProcessorImplementation, EngineContext *>
        , public PopulationObserver
        , public RenderObserver
        , public Processor
    {
    public:
        struct ParticleData
        {
            Math::Float3 origin;
            Math::Float2 offset;
            float age, death;
            float angle, spin;
            float size;

            ParticleData(void)
                : age(0.0f)
                , death(0.0f)
                , angle(0.0f)
                , spin(0.0f)
                , size(1.0f)
            {
            }
        };

        struct EmitterData
            : public Shapes::AlignedBox
        {
            MaterialHandle material;
            ResourceHandle colorMap;
            std::uniform_real_distribution<float> lifeExpectancy;
            std::uniform_real_distribution<float> size;
            std::vector<ParticleData> particles;
        };

        struct Properties
        {
            union
            {
                uint64_t value;
                struct
                {
                    uint16_t buffer;
                    ResourceHandle colorMap;
                    MaterialHandle material;
                };
            };

            Properties(void)
                : value(0)
            {
            }

            Properties(MaterialHandle material, ResourceHandle colorMap)
                : material(material)
                , colorMap(colorMap)
                , buffer(0)
            {
            }

            std::size_t operator()(const Properties &properties) const
            {
                return std::hash<uint64_t>()(properties.value);
            }

            bool operator == (const Properties &properties) const
            {
                return (value == properties.value);
            }
        };

    private:
        Population *population;
        uint32_t updateHandle;
        PluginResources *resources;
        Render *render;

        PluginHandle plugin;
        ResourceHandle particleBuffer;

        ShuntingYard shuntingYard;
        typedef std::unordered_map<Entity *, EmitterData> DataEntityMap;
        DataEntityMap entityDataList;

        typedef concurrency::concurrent_unordered_multimap<Properties, const DataEntityMap::value_type *, Properties> VisibleList;
        VisibleList visibleList;

    public:
        ParticlesProcessorImplementation(Context *context, EngineContext *engine)
            : ContextRegistration(context)
            , population(engine->getPopulation())
            , updateHandle(0)
            , resources(engine->getResources())
            , render(engine->getRender())
        {
            population->addObserver((PopulationObserver *)this);
            updateHandle = population->setUpdatePriority(this, 60);
            render->addObserver((RenderObserver *)this);

            plugin = resources->loadPlugin(L"particles");

            particleBuffer = resources->createBuffer(nullptr, sizeof(ParticleData), ParticleBufferCount, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
        }

        ~ParticlesProcessorImplementation(void)
        {
            render->removeObserver((RenderObserver *)this);
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
                population->removeObserver((PopulationObserver *)this);
            }
        }

        // PopulationObserver
        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
            onFree();
        }

        void onFree(void)
        {
            entityDataList.clear();
        }

        template <float const &(*OPERATION)(float const &, float const &)>
        struct combinable
            : public concurrency::combinable<float>
        {
            combinable(float defaultValue)
                : concurrency::combinable<float>([&] {return defaultValue; })
            {
            }

            void set(float value)
            {
                auto &localValue = local();
                localValue = OPERATION(value, localValue);
            }

            float get(void)
            {
                return combine([](float left, float right) { return OPERATION(left, right); });
            }
        };

        void onEntityCreated(Entity *entity)
        {
            GEK_REQUIRE(resources);
            GEK_REQUIRE(entity);

            if (entity->hasComponents<ParticlesComponent, TransformComponent>())
            {
                auto &particlesComponent = entity->getComponent<ParticlesComponent>();
                auto &transformComponent = entity->getComponent<TransformComponent>();

                auto &emitter = entityDataList[entity];
                emitter.particles.resize(particlesComponent.density);
                emitter.material = resources->loadMaterial(String(L"Particles\\%v", particlesComponent.material));
                emitter.colorMap = resources->loadTexture(String(L"Particles\\%v", particlesComponent.colorMap), 0);
                emitter.lifeExpectancy = std::uniform_real_distribution<float>(particlesComponent.lifeExpectancy.x, particlesComponent.lifeExpectancy.y);
                emitter.size = std::uniform_real_distribution<float>(particlesComponent.size.x, particlesComponent.size.y);
                concurrency::parallel_for_each(emitter.particles.begin(), emitter.particles.end(), [&](auto &particle) -> void
                {
                    particle.age = emitter.lifeExpectancy(mersineTwister);
                    particle.death = emitter.lifeExpectancy(mersineTwister);
                    particle.angle = (zeroToOne(mersineTwister) * Math::Pi * 2.0f);
                    particle.origin = transformComponent.position;
                    particle.offset.x = std::cos(particle.angle);
                    particle.offset.y = -std::sin(particle.angle);
                    particle.offset *= halfToOne(mersineTwister);
                    particle.size = emitter.size(mersineTwister);
                });
            }
        }

        void onEntityDestroyed(Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto dataEntityIterator = entityDataList.find(entity);
            if (dataEntityIterator != entityDataList.end())
            {
                entityDataList.erase(dataEntityIterator);
            }
        }

        void onUpdate(uint32_t handle, bool isIdle)
        {
            if (!isIdle)
            {
                float frameTime = population->getFrameTime();
                concurrency::parallel_for_each(entityDataList.begin(), entityDataList.end(), [&](DataEntityMap::value_type &dataEntity) -> void
                {
                    Entity *entity = dataEntity.first;
                    EmitterData &emitter = dataEntity.second;
                    auto &transformComponent = entity->getComponent<TransformComponent>();

                    combinable<std::min<float>> minimum[3] = { (Math::Infinity), (Math::Infinity), (Math::Infinity) };
                    combinable<std::max<float>> maximum[3] = { (-Math::Infinity), (-Math::Infinity), (-Math::Infinity) };
                    concurrency::parallel_for_each(emitter.particles.begin(), emitter.particles.end(), [&](auto &particle) -> void
                    {
                        particle.age += frameTime;
                        if (particle.age >= particle.death)
                        {
                            particle.age = 0.0f;
                            particle.death = emitter.lifeExpectancy(mersineTwister);
                            particle.spin = (negativeOneToOne(mersineTwister) * Math::Pi);
                            particle.angle = (zeroToOne(mersineTwister) * Math::Pi * 2);
                            particle.origin = transformComponent.position;
                            particle.offset.x = std::cos(particle.angle);
                            particle.offset.y = -std::sin(particle.angle);
                            particle.offset *= halfToOne(mersineTwister);
                            particle.size = emitter.size(mersineTwister);
                        }

                        particle.angle += (particle.spin * frameTime);
                        minimum[0].set(particle.origin.x - 1.0f);
                        minimum[1].set(particle.origin.y);
                        minimum[2].set(particle.origin.z - 1.0f);
                        maximum[0].set(particle.origin.x + 1.0f);
                        maximum[1].set(particle.origin.y + emitter.lifeExpectancy.max());
                        maximum[2].set(particle.origin.z + 1.0f);
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
        static void drawCall(RenderContext *renderContext, PluginResources *resources, ResourceHandle colorMap, VisibleList::iterator begin, VisibleList::iterator end, ResourceHandle particleBuffer)
        {
            GEK_REQUIRE(renderContext);
            GEK_REQUIRE(resources);

            resources->setResource(renderContext->vertexPipeline(), particleBuffer, 0);
            resources->setResource(renderContext->vertexPipeline(), colorMap, 1);

            uint32_t bufferCopied = 0;
            ParticleData *bufferData = nullptr;
            resources->mapBuffer(particleBuffer, (void **)&bufferData);
            for (auto emitterIterator = begin; emitterIterator != end; ++emitterIterator)
            {
                const auto &emitter = emitterIterator->second->second;

                uint32_t particlesCopied = 0;
                uint32_t particlesCount = emitter.particles.size();
                const ParticleData *particleData = emitter.particles.data();
                while (particlesCopied < particlesCount)
                {
                    uint32_t bufferRemaining = (ParticleBufferCount - bufferCopied);
                    uint32_t particlesRemaining = (particlesCount - particlesCopied);
                    uint32_t copyCount = std::min(bufferRemaining, particlesRemaining);
                    memcpy(&bufferData[bufferCopied], &particleData[particlesCopied], (sizeof(ParticleData) * copyCount));

                    bufferCopied += copyCount;
                    particlesCopied += copyCount;
                    if (bufferCopied >= ParticleBufferCount)
                    {
                        resources->unmapBuffer(particleBuffer);
                        renderContext->getContext()->drawPrimitive((ParticleBufferCount * 6), 0);
                        resources->mapBuffer(particleBuffer, (void **)&bufferData);
                        bufferCopied = 0;
                    }
                };
            }

            resources->unmapBuffer(particleBuffer);
            if (bufferCopied > 0)
            {
                renderContext->getContext()->drawPrimitive((bufferCopied * 6), 0);
            }
        }

        void onRenderScene(Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum)
        {
            GEK_REQUIRE(cameraEntity);
            GEK_REQUIRE(viewFrustum);

            visibleList.clear();
            concurrency::parallel_for_each(entityDataList.begin(), entityDataList.end(), [&](auto &dataEntity) -> void
            {
                Entity *entity = dataEntity.first;
                const EmitterData &emitter = dataEntity.second;
                if (viewFrustum->isVisible(emitter))
                {
                    visibleList.insert(std::make_pair(Properties(emitter.material, emitter.colorMap), &dataEntity));
                    //render->queueDrawCall(plugin, emitter.material, std::bind(drawCall, std::placeholders::_1, resources, entity, emitter, particleBuffer));
                }
            });

            for (auto propertiesIterator = visibleList.begin(); propertiesIterator != visibleList.end(); )
            {
                auto emittersIterator = visibleList.equal_range(propertiesIterator->first);
                render->queueDrawCall(plugin, propertiesIterator->first.material, std::bind(drawCall, std::placeholders::_1, resources, propertiesIterator->first.colorMap, emittersIterator.first, emittersIterator.second, particleBuffer));
                propertiesIterator = emittersIterator.second;
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(ParticlesProcessorImplementation)
}; // namespace Gek

