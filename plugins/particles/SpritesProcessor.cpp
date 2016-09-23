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
static std::uniform_real_distribution<float> emitterCreateAge(0.0f, 1.0f);
static std::uniform_real_distribution<float> emitterSpawnAge(0.75f, 1.25f);
static std::uniform_real_distribution<float> emitterForce(1.0f, 5.0f);
static std::uniform_real_distribution<float> fullCircle(0.0f, (Gek::Math::Pi * 2.0f));

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Emitter)
        {
            String type;
            uint32_t density;

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
                saveParameter(componentData, nullptr, type);
                saveParameter(componentData, L"density", density);
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
                type = loadParameter(componentData, nullptr, String(L"explosion"));
				density = loadParameter(componentData, L"density", 100);
            }
        };
    }; // namespace Components

    namespace Sprites
    {
        GEK_CONTEXT_USER(Emitter)
            , public Plugin::ComponentMixin<Components::Emitter>
        {
        public:
            Emitter(Context *context)
                : ContextRegistration(context)
            {
            }

            // Plugin::Component
            const wchar_t * const getName(void) const
            {
                return L"sprites_emitter";
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
                float size;
                float age;
                Math::Color color;
                float buffer[2];
            };

            struct EmitterData : public Shapes::AlignedBox
            {
                MaterialHandle material;
                std::vector<Sprite> spritesList;
                std::function<Shapes::AlignedBox(const Plugin::Entity *, std::vector<Sprite> &, float)> update;
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

                if (entity->hasComponents<Components::Emitter, Components::Transform>())
                {
                    const auto &emitterComponent = entity->getComponent<Components::Emitter>();
                    const auto &transformComponent = entity->getComponent<Components::Transform>();
                    auto &emitter = entityEmitterMap.insert(std::make_pair(entity, EmitterData())).first->second;
                    emitter.spritesList.resize(emitterComponent.density);
                    if (emitterComponent.type.compareNoCase(L"explosion") == 0)
                    {
                        emitter.material = resources->loadMaterial(L"Particles\\Explosion");
                        concurrency::parallel_for_each(emitter.spritesList.begin(), emitter.spritesList.end(), [&](Sprite &sprite) -> void
                        {
                            sprite.position = transformComponent.position;
                            sprite.velocity.set(0.0f, 0.0f, 0.0f);
                            sprite.angle = 0.0f;
                            sprite.torque = 0.0f;
                            sprite.size = 1.0f;
                            sprite.age = 0.0f;
                            sprite.color.set(1.0f, 1.0f, 1.0f, 1.0f);
                        });

                        emitter.update = [](const Plugin::Entity *entity, std::vector<Sprite> &spritesList, float frameTime) -> Shapes::AlignedBox
                        {
                            combinable<std::min<float>> minimum[3] = { (+Math::Infinity), (+Math::Infinity), (+Math::Infinity) };
                            combinable<std::max<float>> maximum[3] = { (-Math::Infinity), (-Math::Infinity), (-Math::Infinity) };
                            concurrency::parallel_for_each(spritesList.begin(), spritesList.end(), [&](Sprite &sprite) -> void
                            {
                                const auto &transformComponent = entity->getComponent<Components::Transform>();
                                sprite.position += (sprite.velocity * frameTime);
                                sprite.angle += (sprite.torque * frameTime);
                                sprite.age += frameTime;

                                for (uint32_t axis = 0; axis < 3; axis++)
                                {
                                    minimum[axis].set(sprite.position[axis] - 1.0f);
                                    maximum[axis].set(sprite.position[axis] + 1.0f);
                                }
                            });

                            return Shapes::AlignedBox(Math::Float3(minimum[0].get(), minimum[1].get(), minimum[2].get()),
                                                      Math::Float3(maximum[0].get(), maximum[1].get(), maximum[2].get()));
                        };
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
                        (Shapes::AlignedBox &)emitter = emitter.update(entity, emitter.spritesList, frameTime);
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
        }; // namespace Sprites

        GEK_REGISTER_CONTEXT_USER(Emitter)
        GEK_REGISTER_CONTEXT_USER(EmitterProcessor)
    };
}; // namespace Gek