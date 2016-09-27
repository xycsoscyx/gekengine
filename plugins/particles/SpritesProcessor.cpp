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

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Explosion)
        {
            float strength;

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
                saveParameter(componentData, L"strength", strength);
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
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
            static std::random_device randomDevice;
            static std::mt19937 mersineTwister;

            __declspec(align(16))
            struct Sprite
            {
                Math::Float3 position;
                Math::Float3 velocity;
                float angle;
                float torque;
                float halfSize;
                float age;
                float buffer[2];

                Sprite(void)
                    : halfSize(0.0f)
                {
                }
            };

            struct EmitterData
            {
                Shapes::AlignedBox box;
                MaterialHandle material;
                std::vector<Sprite> spritesList;

                uint32_t tail;
                std::function<void(const Plugin::Entity *, EmitterData &, float)> update;
            };

        private:
            Plugin::Population *population;
            Plugin::Resources *resources;
            Plugin::Renderer *renderer;

            VisualHandle visual;
            Video::BufferPtr spritesBuffer;

            using EntityEmitterMap = std::unordered_map<Plugin::Entity *, std::vector<EmitterData>>;
            EntityEmitterMap entityEmitterMap;

            using VisibleMap = concurrency::concurrent_unordered_multimap<MaterialHandle, const EmitterData *>;
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
                    static const Math::Float3 gravity(0.0f, -32.174f, 0.0f);
                    static const auto smoke = [this](const Plugin::Entity *entity, EmitterData &emitter) -> void
                    {
                        static const auto update = [](const Plugin::Entity *entity, EmitterData &emitter, float frameTime) -> void
                        {
                            auto &sprite = emitter.spritesList.front();
                            emitter.spritesList[emitter.tail++] = sprite;
                            if (emitter.tail >= emitter.spritesList.size())
                            {
                                emitter.tail = 1;
                            }

                            sprite.position += (sprite.velocity * frameTime);

                            combineMinimum minimum[3];
                            combineMaximum maximum[3];
                            concurrency::parallel_for_each((emitter.spritesList.begin() + 1), emitter.spritesList.end(), [&emitter, frameTime, &minimum, &maximum](Sprite &sprite) -> void
                            {
                                sprite.age += frameTime;
                                sprite.halfSize += frameTime;
                                sprite.angle += (sprite.torque * frameTime);
                                //sprite.position -= (gravity * frameTime);
                                for (uint32_t axis = 0; axis < 3; axis++)
                                {
                                    minimum[axis].set(sprite.position[axis] - sprite.halfSize);
                                    maximum[axis].set(sprite.position[axis] + sprite.halfSize);
                                }
                            });

                            emitter.box.minimum.x = minimum[0].get();
                            emitter.box.minimum.y = minimum[1].get();
                            emitter.box.minimum.z = minimum[2].get();
                            emitter.box.maximum.x = maximum[0].get();
                            emitter.box.maximum.y = maximum[1].get();
                            emitter.box.maximum.z = maximum[2].get();
                        };

                        const auto &explosionComponent = entity->getComponent<Components::Explosion>();
                        const auto &transformComponent = entity->getComponent<Components::Transform>();

                        static const std::uniform_real_distribution<float> spawnDirection(-1.0f, 1.0f);
                        static const std::uniform_real_distribution<float> spawnAzimuthal(0.0f, (Gek::Math::Pi * 2.0f));
                        static const std::uniform_real_distribution<float> spawnZenith(0.0f, 1.0f);
                        static const std::uniform_real_distribution<float> spawnCircle(-Gek::Math::Pi, Gek::Math::Pi);
                        static const std::uniform_real_distribution<float> spawnStrength(0.5f, 1.0f);

                        emitter.tail = 1;
                        emitter.update = update;
                        emitter.material = resources->loadMaterial(L"Sprites\\Smoke");
                        emitter.spritesList.resize(1000);

                        float theta = spawnAzimuthal(mersineTwister);
                        float phi = acos(spawnDirection(mersineTwister));

                        auto &sprite = emitter.spritesList.front();
                        sprite.position = transformComponent.position;
                        sprite.velocity.x = std::sin(phi) * std::sin(theta);
                        sprite.velocity.y = -std::sin(phi) * std::cos(theta);
                        sprite.velocity.z = std::cos(phi);
                        sprite.velocity *= (spawnStrength(mersineTwister) * explosionComponent.strength);
                        sprite.angle = spawnCircle(mersineTwister);
                        sprite.torque = spawnCircle(mersineTwister);
                        sprite.halfSize = 0.1f;
                        sprite.age = 0.0f;
                    };

                    static const auto spark = [this](const Plugin::Entity *entity, EmitterData &emitter) -> void
                    {
                        static const auto update = [](const Plugin::Entity *entity, EmitterData &emitter, float frameTime) -> void
                        {
                            auto &sprite = emitter.spritesList.front();
                            emitter.spritesList[emitter.tail++] = sprite;
                            if (emitter.tail >= emitter.spritesList.size())
                            {
                                emitter.tail = 1;
                            }

                            sprite.position += (sprite.velocity * frameTime);
                            sprite.velocity += (gravity * frameTime);

                            combineMinimum minimum[3];
                            combineMaximum maximum[3];
                            concurrency::parallel_for_each((emitter.spritesList.begin() + 1), emitter.spritesList.end(), [&emitter, frameTime, &minimum, &maximum](Sprite &sprite) -> void
                            {
                                sprite.age += frameTime;
                                sprite.angle += (sprite.torque * frameTime);
                                for (uint32_t axis = 0; axis < 3; axis++)
                                {
                                    minimum[axis].set(sprite.position[axis] - sprite.halfSize);
                                    maximum[axis].set(sprite.position[axis] + sprite.halfSize);
                                }
                            });

                            emitter.box.minimum.x = minimum[0].get();
                            emitter.box.minimum.y = minimum[1].get();
                            emitter.box.minimum.z = minimum[2].get();
                            emitter.box.maximum.x = maximum[0].get();
                            emitter.box.maximum.y = maximum[1].get();
                            emitter.box.maximum.z = maximum[2].get();
                        };

                        const auto &explosionComponent = entity->getComponent<Components::Explosion>();
                        const auto &transformComponent = entity->getComponent<Components::Transform>();

                        static const std::uniform_real_distribution<float> spawnDirection(-1.0f, 1.0f);
                        static const std::uniform_real_distribution<float> spawnAzimuthal(0.0f, (Gek::Math::Pi * 2.0f));
                        static const std::uniform_real_distribution<float> spawnZenith(0.0f, 1.0f);
                        static const std::uniform_real_distribution<float> spawnCircle(-Gek::Math::Pi, Gek::Math::Pi);
                        static const std::uniform_real_distribution<float> spawnStrength(1.0f, 2.0f);

                        emitter.tail = 1;
                        emitter.update = update;
                        emitter.material = resources->loadMaterial(L"Sprites\\Spark");
                        emitter.spritesList.resize(1000);

                        float theta = spawnAzimuthal(mersineTwister);
                        float phi = acos(spawnDirection(mersineTwister));

                        auto &sprite = emitter.spritesList.front();
                        sprite.position = transformComponent.position;
                        sprite.velocity.x = std::sin(phi) * std::sin(theta);
                        sprite.velocity.y = -std::sin(phi) * std::cos(theta);
                        sprite.velocity.z = std::cos(phi);
                        sprite.velocity *= (spawnStrength(mersineTwister) * explosionComponent.strength);
                        sprite.angle = spawnCircle(mersineTwister);
                        sprite.torque = spawnCircle(mersineTwister);
                        sprite.halfSize = 0.1f;
                        sprite.age = 0.0f;
                    };

                    if (entity->hasComponent<Components::Explosion>())
                    {
                        auto &entityEmitterList = entityEmitterMap[entity];

                        entityEmitterList.resize(30);
                        for (uint32_t index = 0; index < 20; index++)
                        {
                            smoke(entity, entityEmitterList[index]);
                        }

                        for (uint32_t index = 20; index < 30; index++)
                        {
                            spark(entity, entityEmitterList[index]);
                        }
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
                    const float frameTime = population->getFrameTime() * 0.1f;
                    concurrency::parallel_for_each(entityEmitterMap.begin(), entityEmitterMap.end(), [&](auto &entityEmitterPair) -> void
                    {
                        const Plugin::Entity *entity = entityEmitterPair.first;
                        concurrency::parallel_for_each(entityEmitterPair.second.begin(), entityEmitterPair.second.end(), [&](auto &emitter) -> void
                        {
                            emitter.update(entity, emitter, frameTime);
                        });
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
                    const auto &emitter = *emitterSearch->second;

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
                    concurrency::parallel_for_each(entityEmitterPair.second.begin(), entityEmitterPair.second.end(), [&](auto &emitter) -> void
                    {
                        if (viewFrustum.isVisible(emitter.box))
                        {
                            visibleMap.insert(std::make_pair(emitter.material, &emitter));
                        }
                    });
                });

                for (auto propertiesSearch = visibleMap.begin(); propertiesSearch != visibleMap.end(); )
                {
                    const auto emittersRange = visibleMap.equal_range(propertiesSearch->first);
                    renderer->queueDrawCall(visual, propertiesSearch->first, std::bind(drawCall, std::placeholders::_1, resources, emittersRange.first, emittersRange.second, spritesBuffer.get()));
                    propertiesSearch = emittersRange.second;
                }
            }
        };

        std::random_device EmitterProcessor::randomDevice;
        std::mt19937 EmitterProcessor::mersineTwister(randomDevice());

        GEK_REGISTER_CONTEXT_USER(Explosion)
        GEK_REGISTER_CONTEXT_USER(EmitterProcessor)
    }; // namespace Sprites
}; // namespace Gek