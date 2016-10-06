#include "GEK\Utility\String.h"
#include "GEK\Utility\ThreadPool.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include <concurrent_queue.h>
#include <ppl.h>
#include <map>

namespace Gek
{
    namespace Implementation
    {
        class Entity
            : public Editor::Entity
        {
        private:
            std::unordered_map<std::type_index, std::unique_ptr<Plugin::Component::Data>> componentsMap;

        public:
            void addComponent(Plugin::Component *component, std::unique_ptr<Plugin::Component::Data> &&data)
            {
                componentsMap[component->getIdentifier()] = std::move(data);
            }

            void removeComponent(const std::type_index &type)
            {
                auto componentSearch = componentsMap.find(type);
                if (componentSearch != componentsMap.end())
                {
                    componentsMap.erase(componentSearch);
                }
            }

            // Editor::Entity
            std::unordered_map<std::type_index, std::unique_ptr<Plugin::Component::Data>> &getComponentsMap(void)
            {
                return componentsMap;
            }

            // Plugin::Entity
            bool hasComponent(const std::type_index &type) const
            {
                return (componentsMap.count(type) > 0);
            }

			Plugin::Component::Data *getComponent(const std::type_index &type)
			{
				auto componentSearch = componentsMap.find(type);
				if (componentSearch == componentsMap.end())
				{
					throw ComponentNotFound();
				}

				return componentSearch->second.get();
			}

			const Plugin::Component::Data *getComponent(const std::type_index &type) const
			{
				auto componentSearch = componentsMap.find(type);
				if (componentSearch == componentsMap.end())
				{
					throw ComponentNotFound();
				}

				return componentSearch->second.get();
			}
		};

        GEK_CONTEXT_USER(Population, Plugin::Core *)
            , public Editor::Population
        {
        private:
            Plugin::Core *core;

            float worldTime;
            float frameTime;

            std::unordered_map<String, std::type_index> componentNamesMap;
            std::unordered_map<std::type_index, Plugin::ComponentPtr> componentsMap;

            ThreadPool loadPool;
            std::future<bool> loaded;
            concurrency::concurrent_queue<std::function<void(void)>> entityQueue;
            using EntityMap = std::unordered_map<String, Plugin::EntityPtr>;
            EntityMap entityMap;

            uint32_t uniqueEntityIdentifier = 0;

        public:
            Population(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , loadPool(1)
            {
                GEK_REQUIRE(core);

                getContext()->listTypes(L"ComponentType", [&](const wchar_t *className) -> void
                {
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className));
                    auto componentSearch = componentsMap.insert(std::make_pair(component->getIdentifier(), component));
                    if (componentSearch.second)
                    {
                        if (!componentNamesMap.insert(std::make_pair(component->getName(), component->getIdentifier())).second)
                        {
                            componentsMap.erase(componentSearch.first);
                        }
                    }
                    else
                    {
                    }
                });
            }

            ~Population(void)
            {
                entityMap.clear();
                componentNamesMap.clear();
                componentsMap.clear();
            }

            // Editor::Population
            EntityMap &getEntityMap(void)
            {
                return entityMap;
            }

            Editor::Component *getComponent(const std::type_index &type)
            {
                auto componentsSearch = componentsMap.find(type);
                if (componentsSearch != componentsMap.end())
                {
                    return dynamic_cast<Editor::Component *>(componentsSearch->second.get());
                }
                
                return nullptr;
            }

            // Plugin::Population
            float getFrameTime(void) const
            {
                return frameTime;
            }

            float getWorldTime(void) const
            {
                return worldTime;
            }

            void update(bool isBackgroundProcess, float frameTime)
            {
                Plugin::PopulationStep::State state = Plugin::PopulationStep::State::Unknown;
                if (loaded.valid() && loaded.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    state = Plugin::PopulationStep::State::Loading;
                }
                else if(isBackgroundProcess)
                {
                    state = Plugin::PopulationStep::State::Idle;
                }
                else
                {
                    state = Plugin::PopulationStep::State::Active;
                    this->frameTime = frameTime;
                    this->worldTime += frameTime;
                }
                
                Sequencer::sendUpdate(&Plugin::PopulationStep::onUpdate, state);

                std::function<void(void)> entityAction;
                while (entityQueue.try_pop(entityAction))
                {
                    entityAction();
                };
            }

            void load(const wchar_t *populationName)
            {
                GEK_REQUIRE(populationName);

                if (loaded.valid())
                {
                    loaded.get();
                }

                loaded = loadPool.enqueue([this, populationName = String(populationName)](void) -> bool
                {
                    try
                    {
                        entityMap.clear();
                        entityQueue.clear();

                        sendShout(&Plugin::PopulationListener::onLoadBegin);

                        Xml::Node worldNode = Xml::load(getContext()->getFileName(L"data\\scenes", populationName).append(L".xml"), L"world");

                        auto &prefabsNode = worldNode.getChild(L"prefabs");
                        for(auto &entityNode : worldNode.getChild(L"population").children)
                        {
                            std::unordered_map<String, Xml::Leaf> entityPrefabMap;
                            prefabsNode.findChild(entityNode.getAttribute(L"prefab"), [&](Xml::Node &prefabNode) -> void
                            {
                                for (auto &prefabComponentNode : prefabNode.children)
                                {
                                    entityPrefabMap[prefabComponentNode.type] = prefabComponentNode;
                                }
                            });

                            auto entity(std::make_shared<Entity>());
                            for (auto &componentNode : entityNode.children)
                            {
                                Xml::Leaf componentData(componentNode.type);
                                if (entityPrefabMap.count(componentNode.type) > 0)
                                {
                                    auto &prefabNode = entityPrefabMap[componentNode.type];
                                    componentData.attributes = prefabNode.attributes;
                                    componentData.text = prefabNode.text;
                                }

                                if (!componentNode.text.empty())
                                {
                                    componentData.text = componentNode.text;
                                }

                                for (auto &attribute : componentNode.attributes)
                                {
                                    componentData.attributes[attribute.first] = attribute.second;
                                }
                                
                                addComponent(entity.get(), componentData);
                            }

                            if (entityNode.attributes.count(L"name") > 0)
                            {
                                entityMap[entityNode.attributes[L"name"]] = std::move(entity);
                            }
                            else
                            {
                                auto name(String::create(L"unnamed_%v", ++uniqueEntityIdentifier));
                                entityMap[name] = std::move(entity);
                            }
                        }

                        frameTime = 0.0f;
                        worldTime = 0.0f;
                        sendShout(&Plugin::PopulationListener::onLoadSucceeded);
                    }
                    catch (const std::exception &)
                    {
                        sendShout(&Plugin::PopulationListener::onLoadFailed);
                    };

                    return true;
                });
            }

            void save(const wchar_t *populationName)
            {
                GEK_REQUIRE(populationName);
            }

            Plugin::Entity *createEntity(const wchar_t *entityName)
            {
                std::shared_ptr<Entity> entity(std::make_shared<Entity>());
                sendShout(&Plugin::PopulationListener::onEntityCreated, entity.get(), entityName);
                entityQueue.push([this, entityName = String(entityName), entity](void) -> void
                {
                    entityMap.insert(std::make_pair((entityName ? entityName : String::create(L"unnamed_%v", ++uniqueEntityIdentifier)), entity));
                });

                return entity.get();
            }

            void killEntity(Plugin::Entity *entity)
            {
                sendShout(&Plugin::PopulationListener::onEntityDestroyed, entity);
                entityQueue.push([this, entity](void) -> void
                {
                    auto entitySearch = std::find_if(entityMap.begin(), entityMap.end(), [entity](const EntityMap::value_type &entitySearch) -> bool
                    {
                        return (entitySearch.second.get() == entity);
                    });

                    if (entitySearch != entityMap.end())
                    {
                        entityMap.erase(entitySearch);
                    }
                });
            }

            void addComponent(Entity *entity, const Xml::Leaf &componentData)
            {
                GEK_REQUIRE(entity);

                auto componentNameSearch = componentNamesMap.find(componentData.type);
                if (componentNameSearch != componentNamesMap.end())
                {
                    std::type_index componentIdentifier(componentNameSearch->second);
                    auto componentSearch = componentsMap.find(componentIdentifier);
                    if (componentSearch != componentsMap.end())
                    {
                        Plugin::Component *componentManager = componentSearch->second.get();
                        auto component(componentManager->create());
                        componentManager->load(component.get(), componentData);

                        entity->addComponent(componentManager, std::move(component));
                        sendShout(&Plugin::PopulationListener::onComponentAdded, static_cast<Plugin::Entity *>(entity), componentIdentifier);
                    }
                }
            }

            void addComponent(Plugin::Entity *entity, const Xml::Leaf &componentData)
            {
                addComponent(static_cast<Entity *>(entity), componentData);
            }

            void removeComponent(Plugin::Entity *entity, const std::type_index &type)
            {
                GEK_REQUIRE(entity);
                sendShout(&Plugin::PopulationListener::onComponentRemoved, entity, type);
                static_cast<Entity *>(entity)->removeComponent(type);
            }

            void listEntities(std::function<void(Plugin::Entity *, const wchar_t *)> onEntity) const
            {
                concurrency::parallel_for_each(entityMap.begin(), entityMap.end(), [&](auto &entity) -> void
                {
                    onEntity(entity.second.get(), entity.first);
                });
            }
        };

        GEK_REGISTER_CONTEXT_USER(Population);
    }; // namespace Implementation
}; // namespace Gek
