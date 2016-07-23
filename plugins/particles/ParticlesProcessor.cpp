#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
#include "GEK\Utility\ShuntingYard.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Color.h"
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
    namespace Components
    {
        struct Particles
        {
            String material;
            String colorMap;
            Math::Float2 lifeExpectancy;
            Math::Float2 size;
            uint32_t density;

            Particles(void)
            {
            }

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
                saveParameter(componentData, nullptr, material);
                saveParameter(componentData, L"density", density);
                saveParameter(componentData, L"color_map", colorMap);
                saveParameter(componentData, L"life_expectancy", lifeExpectancy);
                saveParameter(componentData, L"size", size);
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
                material = loadParameter(componentData, nullptr, String());
                density = loadParameter(componentData, L"density", 100);
                colorMap = loadParameter(componentData, L"color_map", String());
                lifeExpectancy = loadParameter(componentData, L"life_expectancy", Math::Float2(1.0f, 1.0f));
                size = loadParameter(componentData, L"size", Math::Float2(1.0f, 1.0f));
            }
        };
    }; // namespace Components

    GEK_CONTEXT_USER(Particles)
        , public Plugin::ComponentMixin<Components::Particles>
    {
    public:
        Particles(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"particles";
        }
    };

    GEK_CONTEXT_USER(ParticlesProcessor, Plugin::Core *)
        , public Plugin::PopulationListener
        , public Plugin::RendererListener
        , public Plugin::Processor
    {
    public:
        __declspec(align(16))
        struct Particle
        {
            Math::Float3 origin;
            Math::Float2 offset;
            float age, death;
            float angle, spin;
            float size;

            Particle(void)
                : age(0.0f)
                , death(0.0f)
                , angle(0.0f)
                , spin(0.0f)
                , size(1.0f)
            {
            }
        };

        static const uint32_t ParticleBufferCount = ((64 * 1024) / sizeof(Particle));

        struct Emitter
            : public Shapes::AlignedBox
        {
            const Math::Color &color;
            MaterialHandle material;
            ResourceHandle colorMap;
            std::uniform_real_distribution<float> lifeExpectancy;
            std::uniform_real_distribution<float> size;
            std::vector<Particle> particles;

            Emitter(const Math::Color &color)
                : color(color)
            {
            }
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
        Plugin::Population *population;
        uint32_t updateHandle;
        Plugin::Resources *resources;
        Plugin::Renderer *renderer;

        VisualHandle visual;
        Video::BufferPtr particleBuffer;

        ShuntingYard shuntingYard;
        using EntityEmitterMap = std::unordered_map<Plugin::Entity *, Emitter>;
        EntityEmitterMap entityEmitterMap;

        using VisibleMap = concurrency::concurrent_unordered_multimap<Properties, const EntityEmitterMap::value_type *, Properties>;
        VisibleMap visibleMap;

    public:
        ParticlesProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , population(core->getPopulation())
            , updateHandle(0)
            , resources(core->getResources())
            , renderer(core->getRenderer())
        {
            population->addListener(this);
            updateHandle = population->setUpdatePriority(this, 60);
            renderer->addListener(this);

            visual = resources->loadVisual(L"particles");

            particleBuffer = renderer->getDevice()->createBuffer(sizeof(Particle), ParticleBufferCount, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource, false);
        }

        ~ParticlesProcessor(void)
        {
            renderer->removeListener(this);
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
                population->removeListener(this);
            }
        }

        // Plugin::PopulationListener
        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
            onFree();
        }

        void onFree(void)
        {
            entityEmitterMap.clear();
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

        void onEntityCreated(Plugin::Entity *entity)
        {
            GEK_REQUIRE(resources);
            GEK_REQUIRE(entity);

            if (entity->hasComponents<Components::Particles, Components::Transform>())
            {
                auto &particlesComponent = entity->getComponent<Components::Particles>();
                auto &transformComponent = entity->getComponent<Components::Transform>();

                std::reference_wrapper<const Math::Color> color = Math::Color::White;
                if (entity->hasComponent<Components::Color>())
                {
                    color = entity->getComponent<Components::Color>().value;
                }

                auto &emitter = entityEmitterMap.insert(std::make_pair(entity, Emitter(color))).first->second;
                emitter.particles.resize(particlesComponent.density);
                emitter.material = resources->loadMaterial(String(L"Particles\\%v", particlesComponent.material));
                emitter.colorMap = resources->loadTexture(String(L"Particles\\%v", particlesComponent.colorMap), 0);
                emitter.lifeExpectancy = std::uniform_real_distribution<float>(particlesComponent.lifeExpectancy.x, particlesComponent.lifeExpectancy.y);
                emitter.size = std::uniform_real_distribution<float>(particlesComponent.size.x, particlesComponent.size.y);
                concurrency::parallel_for_each(emitter.particles.begin(), emitter.particles.end(), [&](auto &particle) -> void
                {
                    particle.age = emitter.lifeExpectancy(mersineTwister);
                    particle.death = emitter.lifeExpectancy(mersineTwister);
                    particle.spin = (negativeOneToOne(mersineTwister) * Math::Pi);
                    particle.angle = (zeroToOne(mersineTwister) * Math::Pi * 2.0f);
                    particle.origin = transformComponent.position;
                    particle.offset.x = std::cos(particle.angle);
                    particle.offset.y = -std::sin(particle.angle);
                    particle.offset *= halfToOne(mersineTwister);
                    particle.size = emitter.size(mersineTwister);
                });
            }
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto entitySearch = entityEmitterMap.find(entity);
            if (entitySearch != entityEmitterMap.end())
            {
                entityEmitterMap.erase(entitySearch);
            }
        }

        void onUpdate(uint32_t handle, bool isIdle)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(handle), GEK_PARAMETER(isIdle));
            if (!isIdle)
            {
                float frameTime = population->getFrameTime();
                concurrency::parallel_for_each(entityEmitterMap.begin(), entityEmitterMap.end(), [&](auto &entityEmitterPair) -> void
                {
                    Plugin::Entity *entity = entityEmitterPair.first;
                    Emitter &emitter = entityEmitterPair.second;
                    auto &transformComponent = entity->getComponent<Components::Transform>();

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

        // Plugin::RendererListener
        static void drawCall(Video::Device::Context *deviceContext, Plugin::Resources *resources, ResourceHandle colorMap, VisibleMap::iterator visibleBegin, VisibleMap::iterator visibleEnd, Video::Buffer *particleBuffer)
        {
            GEK_REQUIRE(deviceContext);
            GEK_REQUIRE(resources);

            deviceContext->vertexPipeline()->setResource(particleBuffer, 0);
            resources->setResource(deviceContext->vertexPipeline(), colorMap, 1);

            uint32_t bufferCopied = 0;
            Particle *bufferData = nullptr;
            deviceContext->getDevice()->mapBuffer(particleBuffer, (void **)&bufferData);
            for (auto emitterSearch = visibleBegin; emitterSearch != visibleEnd; ++emitterSearch)
            {
                const auto &emitter = emitterSearch->second->second;

                uint32_t particlesCopied = 0;
                uint32_t particlesCount = emitter.particles.size();
                const Particle *particleData = emitter.particles.data();
                while (particlesCopied < particlesCount)
                {
                    uint32_t bufferRemaining = (ParticleBufferCount - bufferCopied);
                    uint32_t particlesRemaining = (particlesCount - particlesCopied);
                    uint32_t copyCount = std::min(bufferRemaining, particlesRemaining);
                    memcpy(&bufferData[bufferCopied], &particleData[particlesCopied], (sizeof(Particle) * copyCount));

                    bufferCopied += copyCount;
                    particlesCopied += copyCount;
                    if (bufferCopied >= ParticleBufferCount)
                    {
                        deviceContext->getDevice()->unmapBuffer(particleBuffer);
                        deviceContext->drawPrimitive((ParticleBufferCount * 6), 0);
                        deviceContext->getDevice()->mapBuffer(particleBuffer, (void **)&bufferData);
                        bufferCopied = 0;
                    }
                };
            }

            deviceContext->getDevice()->unmapBuffer(particleBuffer);
            if (bufferCopied > 0)
            {
                deviceContext->drawPrimitive((bufferCopied * 6), 0);
            }
        }

        void onRenderScene(Plugin::Entity *cameraEntity, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum)
        {
            GEK_TRACE_SCOPE();
            GEK_REQUIRE(renderer);
            GEK_REQUIRE(cameraEntity);

            visibleMap.clear();
            concurrency::parallel_for_each(entityEmitterMap.begin(), entityEmitterMap.end(), [&](auto &entityEmitterPair) -> void
            {
                Plugin::Entity *entity = entityEmitterPair.first;
                const Emitter &emitter = entityEmitterPair.second;
                if (viewFrustum.isVisible(emitter))
                {
                    visibleMap.insert(std::make_pair(Properties(emitter.material, emitter.colorMap), &entityEmitterPair));
                }
            });

            for (auto propertiesSearch = visibleMap.begin(); propertiesSearch != visibleMap.end(); )
            {
                auto emittersRange = visibleMap.equal_range(propertiesSearch->first);
                renderer->queueDrawCall(visual, propertiesSearch->first.material, std::bind(drawCall, std::placeholders::_1, resources, propertiesSearch->first.colorMap, emittersRange.first, emittersRange.second, particleBuffer.get()));
                propertiesSearch = emittersRange.second;
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Particles)
    GEK_REGISTER_CONTEXT_USER(ParticlesProcessor)
}; // namespace Gek