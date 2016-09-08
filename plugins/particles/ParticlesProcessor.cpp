#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Shapes\OrientedBox.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Utility\Allocator.h"
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
static std::uniform_real_distribution<float> emitterCreateAge(0.0f, 0.25f);
static std::uniform_real_distribution<float> emitterSpawnAge(0.75f, 1.25f);
static std::uniform_real_distribution<float> emitterForce(1.0f, 5.0f);
static std::uniform_real_distribution<float> fullCircle(0.0f, (Gek::Math::Pi * 2.0f));

namespace Gek
{
    namespace Components
    {
        struct Particles
        {
			String visual;
			String material;
			String emitter;
			std::vector<String> shapeList;
            float lifeExpectancy;
            uint32_t density;

            Particles(void)
            {
            }

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
				saveParameter(componentData, nullptr, material);
				saveParameter(componentData, L"visual", visual);
				saveParameter(componentData, L"density", density);
				saveParameter(componentData, L"emitter", emitter);
				saveParameter(componentData, L"shape", String::create(shapeList, L','));
				saveParameter(componentData, L"life_expectancy", lifeExpectancy);
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
				material = loadParameter(componentData, nullptr, String());
				visual = loadParameter(componentData, L"visual", String(L"sprite"));
				emitter = loadParameter(componentData, L"emitter", String(L"explosion"));
				shapeList = loadParameter(componentData, L"shape", String("gravity")).split(L',');
				density = loadParameter(componentData, L"density", 100);
				lifeExpectancy = loadParameter(componentData, L"life_expectancy", 5.0f);
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
        , public Plugin::PopulationStep
        , public Plugin::RendererListener
        , public Plugin::Processor
    {
    public:
        __declspec(align(16))
        struct Particle
        {
			Math::Float3 position;
			Math::Float3 velocity;
			float lifeRemaining;
			float angle;
        };

        static const uint32_t ParticleBufferCount = 1000000;

        struct Emitter
            : public Shapes::AlignedBox
        {
			VisualHandle visual;
			MaterialHandle material;

			float lifeExpectancy;
			std::vector<Particle> particles;

            Emitter(void)
				: lifeExpectancy(0.0f)
            {
            }
        };

        struct Properties
        {
            union
            {
                uint32_t value;
                struct
                {
                    uint8_t buffer;
                    MaterialHandle material;
					VisualHandle visual;
				};
            };

            Properties(void)
                : value(0)
            {
            }

            Properties(VisualHandle visual, MaterialHandle material)
				: visual(visual)
                , material(material)
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
        Plugin::Resources *resources;
        Plugin::Renderer *renderer;

        Video::BufferPtr particleBuffer;

        using EntityEmitterMap = std::unordered_map<Plugin::Entity *, Emitter>;
        EntityEmitterMap entityEmitterMap;

        using VisibleMap = concurrency::concurrent_unordered_multimap<Properties, const EntityEmitterMap::value_type *, Properties>;
        VisibleMap visibleMap;

    public:
        ParticlesProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
        {
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

            population->addListener(this);
            population->addStep(this, 60);
            renderer->addListener(this);

            particleBuffer = renderer->getDevice()->createBuffer(sizeof(Particle), ParticleBufferCount, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource, false);
        }

        ~ParticlesProcessor(void)
        {
            renderer->removeListener(this);
            population->removeStep(this);
            population->removeListener(this);
        }

        // Plugin::PopulationListener
        void onLoadBegin(void)
        {
            entityEmitterMap.clear();
        }

        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
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
				const auto &particlesComponent = entity->getComponent<Components::Particles>();
				const auto &transformComponent = entity->getComponent<Components::Transform>();

                auto &emitter = entityEmitterMap.insert(std::make_pair(entity, Emitter())).first->second;
				emitter.visual = resources->loadVisual(particlesComponent.visual);
				emitter.material = resources->loadMaterial(String::create(L"Particles\\%v", particlesComponent.material));
			
				auto emitParticle = [emitter, transformComponent](Particle &particle) -> void
				{
					particle.position = transformComponent.position;
					particle.velocity = Math::Float4x4::createEulerRotation(fullCircle(mersineTwister), fullCircle(mersineTwister), fullCircle(mersineTwister)).nz * emitterForce(mersineTwister);
					particle.lifeRemaining = (emitterCreateAge(mersineTwister) * emitter.lifeExpectancy);
				};

				emitter.lifeExpectancy = particlesComponent.lifeExpectancy;
				emitter.particles.resize(particlesComponent.density);
				concurrency::parallel_for_each(emitter.particles.begin(), emitter.particles.end(), [&](auto &particle) -> void
                {
					emitParticle(particle);
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

        // Plugin::PopulationStep
        void onUpdate(uint32_t order, State state)
        {
            if (state == State::Active)
            {
				const float frameTime = population->getFrameTime();
                concurrency::parallel_for_each(entityEmitterMap.begin(), entityEmitterMap.end(), [&](auto &entityEmitterPair) -> void
                {
					const Plugin::Entity *entity = entityEmitterPair.first;
					const auto &transformComponent = entity->getComponent<Components::Transform>();
					Emitter &emitter = entityEmitterPair.second;

					const auto emitParticle = [emitter = static_cast<const Emitter &>(emitter), transformComponent, frameTime](Particle &particle) -> void
					{
						particle.position = transformComponent.position;
						particle.velocity = Math::Float4x4::createEulerRotation(fullCircle(mersineTwister), fullCircle(mersineTwister), fullCircle(mersineTwister)).nz * emitterForce(mersineTwister);
						particle.lifeRemaining = (emitterSpawnAge(mersineTwister) * emitter.lifeExpectancy);
					};

					const auto updateParticle = [emitter = static_cast<const Emitter &>(emitter), transformComponent, frameTime](Particle &particle) -> void
					{
						static const Math::Float3 gravity(0.0f, -32.174f, 0.0f);
						particle.velocity += (gravity * frameTime);
						particle.position += (particle.velocity * frameTime);
					};

                    combinable<std::min<float>> minimum[3] = { (+Math::Infinity), (+Math::Infinity), (+Math::Infinity) };
                    combinable<std::max<float>> maximum[3] = { (-Math::Infinity), (-Math::Infinity), (-Math::Infinity) };
					concurrency::parallel_for_each(emitter.particles.begin(), emitter.particles.end(), [&](auto &particle) -> void
					{
						particle.lifeRemaining -= frameTime;
						if (particle.lifeRemaining <= 0.0f)
						{
							emitParticle(particle);
						}

						updateParticle(particle);
						minimum[0].set(particle.position.x - 1.0f);
						minimum[1].set(particle.position.y - 1.0f);
						minimum[2].set(particle.position.z - 1.0f);
						maximum[0].set(particle.position.x + 1.0f);
						maximum[1].set(particle.position.y + 1.0f);
						maximum[2].set(particle.position.z + 1.0f);
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
        static void drawCall(Video::Device::Context *deviceContext, Plugin::Resources *resources, const VisibleMap::iterator visibleBegin, const VisibleMap::iterator visibleEnd, Video::Buffer *particleBuffer)
        {
            GEK_REQUIRE(deviceContext);
            GEK_REQUIRE(resources);

            deviceContext->vertexPipeline()->setResource(particleBuffer, 0);

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

        void onRenderScene(const Plugin::Entity *cameraEntity, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum)
        {
            GEK_REQUIRE(renderer);
            GEK_REQUIRE(cameraEntity);

            visibleMap.clear();
            concurrency::parallel_for_each(entityEmitterMap.begin(), entityEmitterMap.end(), [&](auto &entityEmitterPair) -> void
            {
                const Emitter &emitter = entityEmitterPair.second;
                if (viewFrustum.isVisible(emitter))
                {
                    visibleMap.insert(std::make_pair(Properties(emitter.visual, emitter.material), &entityEmitterPair));
                }
            });

            for (auto propertiesSearch = visibleMap.begin(); propertiesSearch != visibleMap.end(); )
            {
                const auto emittersRange = visibleMap.equal_range(propertiesSearch->first);
                renderer->queueDrawCall(propertiesSearch->first.visual, propertiesSearch->first.material, std::bind(drawCall, std::placeholders::_1, resources, emittersRange.first, emittersRange.second, particleBuffer.get()));
                propertiesSearch = emittersRange.second;
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Particles)
    GEK_REGISTER_CONTEXT_USER(ParticlesProcessor)
}; // namespace Gek