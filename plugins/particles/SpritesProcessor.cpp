#include "GEK\Math\Float4x4.hpp"
#include "GEK\Shapes\AlignedBox.hpp"
#include "GEK\Shapes\OrientedBox.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Utility\Allocator.hpp"
#include "GEK\Context\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Components\Color.hpp"
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

            void save(Xml::Leaf &componentData) const
            {
                componentData.attributes[L"strength"] = strength;
            }

            void load(const Xml::Leaf &componentData)
            {
                strength = loadAttribute(componentData, L"strength", 10.0f);
            }
        };
    }; // namespace Components

    namespace Sprites
    {
        GEK_CONTEXT_USER(Explosion)
            , public Plugin::ComponentMixin<Components::Explosion, Edit::Component>
        {
        public:
            Explosion(Context *context)
                : ContextRegistration(context)
            {
            }

            // Edit::Component
            void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
            {
            }

            void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
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
                float life;
                uint32_t frames;

                Sprite(void)
                    : halfSize(0.0f)
                    , age(Math::NegativeInfinity)
                    , life(0.0f)
                    , frames(0)
                {
                }
            };

            struct EmitterData
            {
                Math::Float3 position;
                Math::Float3 velocity;

                MaterialHandle material;
                std::vector<Sprite> spritesList;

                uint32_t tail;
                std::function<void(const Plugin::Entity *, EmitterData &, float)> update;

                EmitterData(void)
                    : tail(0)
                {
                }
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
            void onLoadBegin(const String &populationName)
            {
                entityEmitterMap.clear();
            }

            void onLoadSucceeded(const String &populationName)
            {
                population->listEntities<Components::Explosion, Components::Transform>([&](Plugin::Entity *entity, const wchar_t *) -> void
                {
                    static const Math::Float3 gravity(0.0f, -32.174f, 0.0f);
                    static const std::uniform_real_distribution<float> spawnTheta(0.0f, (Gek::Math::Pi * 2.0f));
                    static const std::uniform_real_distribution<float> spawnPhi(-1.0f, 1.0f);
                    static const std::uniform_real_distribution<float> spawnTorque(0.0f, (Gek::Math::Pi * 0.5f));

                    static const auto smoke = [this](const Plugin::Entity *entity, EmitterData &emitter) -> void
                    {
                        static const auto update = [](const Plugin::Entity *entity, EmitterData &emitter, float frameTime) -> void
                        {
                            static const auto update = [](const Plugin::Entity *entity, EmitterData &emitter, float frameTime) -> void
                            {
                                concurrency::parallel_for_each(emitter.spritesList.begin(), emitter.spritesList.end(), [&emitter, frameTime](Sprite &sprite) -> void
                                {
                                    sprite.age += frameTime;
                                    sprite.halfSize = (Math::saturate(sprite.age / sprite.life) * 1.0f);
                                    sprite.angle += (sprite.torque * frameTime);
                                    sprite.position += (sprite.velocity * frameTime);
                                    sprite.position -= (gravity * 0.1f * frameTime);
                                });
                            };

                            emitter.position += (emitter.velocity * frameTime);
                            if (emitter.tail < emitter.spritesList.size())
                            {
                                auto &sprite = emitter.spritesList[emitter.tail++];
                                sprite.position = emitter.position;
                                sprite.age = 0.0f;
                                update(entity, emitter, frameTime);
                            }
                            else
                            {
                                emitter.update = update;
                            }
                        };

                        const auto &explosionComponent = entity->getComponent<Components::Explosion>();
                        const auto &transformComponent = entity->getComponent<Components::Transform>();

                        static const std::uniform_real_distribution<float> spawnStrength(0.1f, 0.5f);

                        emitter.update = update;
                        emitter.material = resources->loadMaterial(L"Sprites\\Smoke");
                        emitter.spritesList.resize(100);
                        emitter.position = transformComponent.position;
                        float theta = spawnTheta(mersineTwister);
                        float phi = acos(spawnPhi(mersineTwister));
                        emitter.velocity.x = std::sin(phi) * std::sin(theta);
                        emitter.velocity.y = -std::sin(phi) * std::cos(theta);
                        emitter.velocity.z = std::cos(phi);
                        emitter.velocity *= (spawnStrength(mersineTwister) * explosionComponent.strength);
                        concurrency::parallel_for_each(emitter.spritesList.begin(), emitter.spritesList.end(), [transformComponent](Sprite &sprite) -> void
                        {
                            static const std::uniform_real_distribution<float> spawnLife(1.0f, 2.0f);

                            float theta = spawnTheta(mersineTwister);
                            float phi = acos(spawnPhi(mersineTwister));
                            sprite.velocity.x = std::sin(phi) * std::sin(theta);
                            sprite.velocity.y = -std::sin(phi) * std::cos(theta);
                            sprite.velocity.z = std::cos(phi);
                            sprite.velocity *= 0.1f;
                            sprite.angle = spawnTheta(mersineTwister);
                            sprite.torque = spawnTorque(mersineTwister);
                            sprite.life = spawnLife(mersineTwister);
                            sprite.frames = 6;
                        });
                    };

                    static const auto spark = [this](const Plugin::Entity *entity, EmitterData &emitter) -> void
                    {
                        static const auto update = [](const Plugin::Entity *entity, EmitterData &emitter, float frameTime) -> void
                        {
                            emitter.position += (emitter.velocity * frameTime);
                            emitter.velocity += (gravity * frameTime);
                            auto &sprite = emitter.spritesList[emitter.tail++];
                            if (emitter.tail >= emitter.spritesList.size())
                            {
                                emitter.tail = 0;
                            }

                            sprite.position = emitter.position;
                            sprite.age = 0.0f;

                            concurrency::parallel_for_each(emitter.spritesList.begin(), emitter.spritesList.end(), [&emitter, frameTime](Sprite &sprite) -> void
                            {
                                sprite.age += frameTime;
                                sprite.angle += (sprite.torque * frameTime);
                                sprite.position += (sprite.velocity * frameTime);
                            });
                        };

                        const auto &explosionComponent = entity->getComponent<Components::Explosion>();
                        const auto &transformComponent = entity->getComponent<Components::Transform>();

                        static const std::uniform_real_distribution<float> spawnStrength(0.5f, 1.0f);

                        emitter.update = update;
                        emitter.material = resources->loadMaterial(L"Sprites\\Spark");
                        emitter.spritesList.resize(10);
                        emitter.position = transformComponent.position;
                        float theta = spawnTheta(mersineTwister);
                        float phi = acos(spawnPhi(mersineTwister));
                        emitter.velocity.x = std::sin(phi) * std::sin(theta);
                        emitter.velocity.y = -std::sin(phi) * std::cos(theta);
                        emitter.velocity.z = std::cos(phi);
                        emitter.velocity *= (spawnStrength(mersineTwister) * explosionComponent.strength);
                        concurrency::parallel_for_each(emitter.spritesList.begin(), emitter.spritesList.end(), [transformComponent](Sprite &sprite) -> void
                        {
                            static const std::uniform_real_distribution<float> spawnLife(2.0f, 4.0f);

                            float theta = spawnTheta(mersineTwister);
                            float phi = acos(spawnPhi(mersineTwister));
                            sprite.velocity.x = std::sin(phi) * std::sin(theta);
                            sprite.velocity.y = -std::sin(phi) * std::cos(theta);
                            sprite.velocity.z = std::cos(phi);
                            sprite.velocity *= 0.1f;
                            sprite.angle = spawnTheta(mersineTwister);
                            sprite.torque = spawnTorque(mersineTwister);
                            sprite.life = spawnLife(mersineTwister);
                            sprite.halfSize = 0.025f;
                            sprite.frames = 5;
                        });
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
                });
            }

            void onEntityCreated(Plugin::Entity *entity)
            {
                GEK_REQUIRE(resources);
                GEK_REQUIRE(entity);

                if (entity->hasComponent<Components::Transform>())
                {
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
            bool onUpdate(int32_t order, State state)
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

                return true;
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
                        //if (viewFrustum.isVisible(emitter.box))
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