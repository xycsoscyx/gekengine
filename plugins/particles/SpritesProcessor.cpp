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
#include <concurrent_unordered_map.h>
#include <functional>
#include <random>
#include <ppl.h>

static std::random_device randomDevice;
static std::mt19937 mersineTwister(randomDevice());

static std::uniform_real_distribution<float> explosionSpawnDirection(-1.0f, 1.0f);
static std::uniform_real_distribution<float> explosionSpawnAngle(0.0f, (Gek::Math::Pi * 2.0f));
static std::uniform_real_distribution<float> explosionSpawnTorque(-Gek::Math::Pi, Gek::Math::Pi);

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Explosion)
        {
            uint32_t density;
            float strength;

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
                saveParameter(componentData, L"density", density);
                saveParameter(componentData, L"strength", strength);
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
                density = loadParameter(componentData, L"density", 100);
                strength = loadParameter(componentData, L"strength", 10.0f);
            }
        };
    }; // namespace Components

    namespace Sprites
    {
        GEK_CONTEXT_USER(Explosion)
            , public Plugin::ComponentMixin<Components::Explosion>
        {
        public:
            Explosion(Context *context)
                : ContextRegistration(context)
            {
            }

            // Plugin::Component
            const wchar_t * const getName(void) const
            {
                return L"explosion";
            }
        };

        GEK_CONTEXT_USER(EmitterProcessor, Plugin::Core *)
            , public Plugin::PopulationListener
            , public Plugin::PopulationStep
            , public Plugin::RendererListener
            , public Plugin::Processor
        {
        public:
            static const uint32_t SpritesBufferCount = 1000000;

            __declspec(align(16))
            struct Sprite
            {
                Math::Float3 position;
                Math::Float3 velocity;
                float angle;
                float torque;
                float halfSize;
                float age;
                Math::Color color;
                float buffer[2];
            };

            struct EmitterData : public Shapes::AlignedBox
            {
                MaterialHandle material;
                std::vector<Sprite> spritesList;
                std::function<void(const Plugin::Entity *, EmitterData &, float)> update;
            };

        private:
            Plugin::Population *population;
            Plugin::Resources *resources;
            Plugin::Renderer *renderer;

            VisualHandle visual;
            Video::BufferPtr spritesBuffer;

            using EntityEmitterMap = std::unordered_map<Plugin::Entity *, EmitterData>;
            EntityEmitterMap entityEmitterMap;

            using VisibleMap = concurrency::concurrent_unordered_multimap<MaterialHandle, const EntityEmitterMap::value_type *>;
            VisibleMap visibleMap;

        public:
            EmitterProcessor(Context *context, Plugin::Core *core)
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

                visual = resources->loadVisual(L"Sprites");
                spritesBuffer = renderer->getDevice()->createBuffer(sizeof(Sprite), SpritesBufferCount, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource, false);
            }

            ~EmitterProcessor(void)
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

            struct combineMinimum
            {
                concurrency::combinable<float> combinable;

                combineMinimum(void)
                    : combinable([&]
                    {
                        return Math::Infinity;
                    })
                {
                }

                void set(float value)
                {
                    auto &localValue = combinable.local();
                    localValue = std::min<float>(value, localValue);
                }

                float get(void)
                {
                    return combinable.combine([](float left, float right)
                    {
                        return std::min<float>(left, right);
                    });
                }
            };

            struct combineMaximum
            {
                concurrency::combinable<float> combinable;

                combineMaximum(void)
                    : combinable([&]
                    {
                        return Math::NegativeInfinity;
                    })
                {
                }

                void set(float value)
                {
                    auto &localValue = combinable.local();
                    localValue = std::max<float>(value, localValue);
                }

                float get(void)
                {
                    return combinable.combine([](float left, float right)
                    {
                        return std::max<float>(left, right);
                    });
                }
            };

            void onEntityCreated(Plugin::Entity *entity)
            {
                GEK_REQUIRE(resources);
                GEK_REQUIRE(entity);

                if (entity->hasComponent<Components::Transform>())
                {
                    if (entity->hasComponent<Components::Explosion>())
                    {
                        static const auto explosionUpdate = [](const Plugin::Entity *entity, EmitterData &emitter, float frameTime) -> void
                        {
                            combineMinimum minimum[3];
                            combineMaximum maximum[3];
                            const auto &transformComponent = entity->getComponent<Components::Transform>();
                            concurrency::parallel_for_each(emitter.spritesList.begin(), emitter.spritesList.end(), [&emitter, transformComponent, frameTime, &minimum, &maximum](Sprite &sprite) -> void
                            {
                                static const Math::Float3 gravity(0.0f, -32.174f, 0.0f);

                                sprite.position += (sprite.velocity * frameTime);
                                //sprite.velocity += (gravity * frameTime);
                                sprite.angle += (sprite.torque * frameTime);
                                sprite.age += frameTime;

                                for (uint32_t axis = 0; axis < 3; axis++)
                                {
                                    minimum[axis].set(sprite.position[axis] - sprite.halfSize);
                                    maximum[axis].set(sprite.position[axis] + sprite.halfSize);
                                }
                            });

                            emitter.minimum.x = minimum[0].get();
                            emitter.minimum.y = minimum[1].get();
                            emitter.minimum.z = minimum[2].get();
                            emitter.maximum.x = maximum[0].get();
                            emitter.maximum.y = maximum[1].get();
                            emitter.maximum.z = maximum[2].get();
                        };

                        const auto &explosionComponent = entity->getComponent<Components::Explosion>();
                        const auto &transformComponent = entity->getComponent<Components::Transform>();
                        auto &emitter = entityEmitterMap.insert(std::make_pair(entity, EmitterData())).first->second;
                        emitter.material = resources->loadMaterial(L"Particles\\Explosion");
                        emitter.update = explosionUpdate;
                        emitter.spritesList.resize(explosionComponent.density);
                        concurrency::parallel_for_each(emitter.spritesList.begin(), emitter.spritesList.end(), [&](Sprite &sprite) -> void
                        {
                            sprite.position = transformComponent.position;
                            sprite.velocity.x = explosionSpawnDirection(mersineTwister);
                            sprite.velocity.y = explosionSpawnDirection(mersineTwister);
                            sprite.velocity.z = explosionSpawnDirection(mersineTwister);
                            sprite.velocity.normalize();
                            sprite.velocity = 0.0f;
                            sprite.angle = explosionSpawnAngle(mersineTwister);
                            sprite.torque = explosionSpawnTorque(mersineTwister);
                            sprite.halfSize = 0.5f;
                            sprite.age = 0.0f;
                            sprite.color.set(1.0f, 1.0f, 1.0f, 1.0f);
                        });
                    }
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
                        EmitterData &emitter = entityEmitterPair.second;
                        emitter.update(entity, emitter, frameTime);
                    });
                }
            }

            // Plugin::RendererListener
            static void drawCall(Video::Device::Context *deviceContext, Plugin::Resources *resources, const VisibleMap::iterator visibleBegin, const VisibleMap::iterator visibleEnd, Video::Buffer *spritesBuffer)
            {
                GEK_REQUIRE(deviceContext);
                GEK_REQUIRE(resources);

                deviceContext->vertexPipeline()->setResource(spritesBuffer, 0);

                uint32_t bufferCopied = 0;
                Sprite *bufferData = nullptr;
                deviceContext->getDevice()->mapBuffer(spritesBuffer, (void **)&bufferData);
                for (auto emitterSearch = visibleBegin; emitterSearch != visibleEnd; ++emitterSearch)
                {
                    const auto &emitter = emitterSearch->second->second;

                    uint32_t spritesCopied = 0;
                    uint32_t spritesCount = emitter.spritesList.size();
                    const Sprite *spriteData = emitter.spritesList.data();
                    while (spritesCopied < spritesCount)
                    {
                        uint32_t bufferRemaining = (SpritesBufferCount - bufferCopied);
                        uint32_t spritesRemaining = (spritesCount - spritesCopied);
                        uint32_t copyCount = std::min(bufferRemaining, spritesRemaining);
                        memcpy(&bufferData[bufferCopied], &spriteData[spritesCopied], (sizeof(Sprite) * copyCount));

                        bufferCopied += copyCount;
                        spritesCopied += copyCount;
                        if (bufferCopied >= SpritesBufferCount)
                        {
                            deviceContext->getDevice()->unmapBuffer(spritesBuffer);
                            deviceContext->drawPrimitive((SpritesBufferCount * 6), 0);
                            deviceContext->getDevice()->mapBuffer(spritesBuffer, (void **)&bufferData);
                            bufferCopied = 0;
                        }
                    };
                }

                deviceContext->getDevice()->unmapBuffer(spritesBuffer);
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
                    const EmitterData &emitter = entityEmitterPair.second;
                    if (viewFrustum.isVisible(emitter))
                    {
                        visibleMap.insert(std::make_pair(emitter.material, &entityEmitterPair));
                    }
                });

                for (auto propertiesSearch = visibleMap.begin(); propertiesSearch != visibleMap.end(); )
                {
                    const auto emittersRange = visibleMap.equal_range(propertiesSearch->first);
                    renderer->queueDrawCall(visual, propertiesSearch->first, std::bind(drawCall, std::placeholders::_1, resources, emittersRange.first, emittersRange.second, spritesBuffer.get()));
                    propertiesSearch = emittersRange.second;
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Explosion)
        GEK_REGISTER_CONTEXT_USER(EmitterProcessor)
    }; // namespace Sprites
}; // namespace Gek