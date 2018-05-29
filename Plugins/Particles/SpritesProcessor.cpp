#include "GEK/Math/Float4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/XML.hpp"
#include "GEK/Utility/Allocator.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Processor.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Color.hpp"
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

            void save(Xml::Leaf &exportData) const
            {
                exportData.attributes[L"strength"] = strength;
            }

            void load(const Xml::Leaf &importData)
            {
                strength = loadAttribute(importData, L"strength", 10.0f);
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
            , public Plugin::Processor
        {
        public:
            static const uint32_t SpritesBufferCount = 1000000;
            static std::random_device randomDevice;
            static std::mt19937 mersineTwister;

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

                population->onLoadBegin.connect<EmitterProcessor, &EmitterProcessor::onLoadBegin>(this);
                population->onLoadSucceeded.connect<EmitterProcessor, &EmitterProcessor::onLoadSucceeded>(this);
                population->onEntityCreated.connect<EmitterProcessor, &EmitterProcessor::onEntityCreated>(this);
                population->onEntityDestroyed.connect<EmitterProcessor, &EmitterProcessor::onEntityDestroyed>(this);
                population->onUpdate[60].connect<EmitterProcessor, &EmitterProcessor::onUpdate>(this);
                renderer->onRenderScene.connect<EmitterProcessor, &EmitterProcessor::onRenderScene>(this);

                visual = resources->loadVisual(L"Sprites");
                spritesBuffer = renderer->getVideoDevice()->createBuffer(sizeof(Sprite), SpritesBufferCount, Video::BufferType::Structured, Video::BufferFlags::Mappable | Video::BufferFlags::Resource, false);
            }

            ~EmitterProcessor(void)
            {
                renderer->onRenderScene.disconnect<EmitterProcessor, &EmitterProcessor::onRenderScene>(this);
                population->onUpdate[60].disconnect<EmitterProcessor, &EmitterProcessor::onUpdate>(this);
                population->onEntityDestroyed.disconnect<EmitterProcessor, &EmitterProcessor::onEntityDestroyed>(this);
                population->onEntityCreated.disconnect<EmitterProcessor, &EmitterProcessor::onEntityCreated>(this);
                population->onLoadSucceeded.disconnect<EmitterProcessor, &EmitterProcessor::onLoadSucceeded>(this);
                population->onLoadBegin.disconnect<EmitterProcessor, &EmitterProcessor::onLoadBegin>(this);
            }

            // Plugin::Population Slots
            void onLoadBegin(const String &populationName)
            {
                entityEmitterMap.clear();
            }

            void onLoadSucceeded(const String &populationName)
            {
                population->listEntities<Components::Transform, Components::Explosion>([&](Plugin::Entity *entity, const wchar_t *, auto &transformComponent, auto &explosionComponent) -> void
                {
                    static const Math::Float3 gravity(0.0f, -32.174f, 0.0f);
                    static const std::uniform_real_distribution<float> spawnTheta(0.0f, (Gek::Math::Pi * 2.0f));
                    static const std::uniform_real_distribution<float> spawnPhi(-1.0f, 1.0f);
                    static const std::uniform_real_distribution<float> spawnTorque(0.0f, (Gek::Math::Pi * 0.5f));

                    auto smoke = [this, &transformComponent, &explosionComponent](const Plugin::Entity *entity, EmitterData &emitter) -> void
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

                    auto spark = [this, &transformComponent, &explosionComponent](const Plugin::Entity *entity, EmitterData &emitter) -> void
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

            void onEntityCreated(Plugin::Entity *entity, const wchar_t *entityName)
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

            void onUpdate(void)
            {
                GEK_REQUIRE(population);

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

            // Plugin::Renderer Slots
            static void drawCall(Video::Device *videoDevice, Video::Device::Context *videoContext, Plugin::Resources *resources, const VisibleMap::iterator visibleBegin, const VisibleMap::iterator visibleEnd, Video::Buffer *spritesBuffer)
            {
                GEK_REQUIRE(videoContext);
                GEK_REQUIRE(resources);

                videoContext->vertexPipeline()->setResourceList({ spritesBuffer }, 0);

                uint32_t bufferCopied = 0;
                Sprite *bufferData = nullptr;
                videoDevice->mapBuffer(spritesBuffer, bufferData);
                for (auto emitterSearch = visibleBegin; emitterSearch != visibleEnd; ++emitterSearch)
                {
                    auto const &emitter = *emitterSearch->second;

                    uint32_t spritesCopied = 0;
                    uint32_t spritesCount = emitter.spritesList.size();
                    const Sprite *spriteData = emitter.spritesList.data();
                    while (spritesCopied < spritesCount)
                    {
                        uint32_t bufferRemaining = (SpritesBufferCount - bufferCopied);
                        uint32_t spritesRemaining = (spritesCount - spritesCopied);
                        uint32_t copyCount = std::min(bufferRemaining, spritesRemaining);
                        std::copy(&bufferData[bufferCopied], &bufferData[bufferCopied + copyCount], &spriteData[spritesCopied]);

                        bufferCopied += copyCount;
                        spritesCopied += copyCount;
                        if (bufferCopied >= SpritesBufferCount)
                        {
                            videoDevice->unmapBuffer(spritesBuffer);
                            videoContext->drawPrimitive((SpritesBufferCount * 6), 0);
                            videoDevice->mapBuffer(spritesBuffer, bufferData);
                            bufferCopied = 0;
                        }
                    };
                }

                videoDevice->unmapBuffer(spritesBuffer);
                if (bufferCopied > 0)
                {
                    videoContext->drawPrimitive((bufferCopied * 6), 0);
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
                    renderer->queueDrawCall(visual, propertiesSearch->first, std::bind(drawCall, renderer->getVideoDevice(), std::placeholders::_1, resources, emittersRange.first, emittersRange.second, spritesBuffer.get()));
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