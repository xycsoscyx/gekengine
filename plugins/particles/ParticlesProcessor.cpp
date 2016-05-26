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
    static const UINT32 ParticleBufferCount = 1000;

    class ParticlesProcessorImplementation
        : public ContextUserMixin
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
                UINT64 value;
                struct
                {
                    UINT8 buffer;
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
                return std::hash<UINT64>()(properties.value);
            }

            bool operator == (const Properties &properties) const
            {
                return (value == properties.value);
            }
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

        typedef concurrency::concurrent_unordered_multimap<Properties, const DataEntityMap::value_type *, Properties> VisibleList;
        VisibleList visibleList;

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
            ObservableMixin::removeObserver(render, getClass<RenderObserver>());
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
            }

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
            GEK_REQUIRE(initializerContext);

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
                particleBuffer = resources->createBuffer(nullptr, sizeof(ParticleData), ParticleBufferCount, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource);
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

        STDMETHODIMP_(void) onEntityCreated(Entity *entity)
        {
            GEK_REQUIRE(resources);
            GEK_REQUIRE(entity);

            if (entity->hasComponents<ParticlesComponent, TransformComponent>())
            {
                auto &particlesComponent = entity->getComponent<ParticlesComponent>();
                auto &transformComponent = entity->getComponent<TransformComponent>();

                auto &emitter = entityDataList[entity];
                emitter.particles.resize(particlesComponent.density);
                emitter.material = resources->loadMaterial(L"Particles\\" + particlesComponent.material);
                emitter.colorMap = resources->loadTexture(L"Particles\\" + particlesComponent.colorMap, nullptr, 0);
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

        STDMETHODIMP_(void) onEntityDestroyed(Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto dataEntityIterator = entityDataList.find(entity);
            if (dataEntityIterator != entityDataList.end())
            {
                entityDataList.erase(dataEntityIterator);
            }
        }

        STDMETHODIMP_(void) onUpdate(UINT32 handle, bool isIdle)
        {
            if (!isIdle)
            {
                float frameTime = population->getFrameTime();
                concurrency::parallel_for_each(entityDataList.begin(), entityDataList.end(), [&](DataEntityMap::value_type &dataEntity) -> void
                {
                    Entity *entity = dataEntity.first;
                    EmitterData &emitter = const_cast<EmitterData &>(dataEntity.second);
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

            UINT32 bufferCopied = 0;
            ParticleData *bufferData = nullptr;
            resources->mapBuffer(particleBuffer, (void **)&bufferData);
            for (auto emitterIterator = begin; emitterIterator != end; ++emitterIterator)
            {
                const auto &emitter = emitterIterator->second->second;

                UINT32 particlesCopied = 0;
                UINT32 particlesCount = emitter.particles.size();
                const ParticleData *particleData = emitter.particles.data();
                while (particlesCopied < particlesCount)
                {
                    UINT32 bufferRemaining = (ParticleBufferCount - bufferCopied);
                    UINT32 particlesRemaining = (particlesCount - particlesCopied);
                    UINT32 copyCount = std::min(bufferRemaining, particlesRemaining);
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

        STDMETHODIMP_(void) onRenderScene(Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum)
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

    REGISTER_CLASS(ParticlesProcessorImplementation)
}; // namespace Gek

