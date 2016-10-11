#include "GEK\Utility\String.hpp"
#include "GEK\Utility\ThreadPool.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Context\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\Component.hpp"
#include <concurrent_queue.h>
#include <ppl.h>
#include <map>

namespace Gek
{
    namespace Implementation
    {
        class Entity
            : public Edit::Entity
        {
        private:
            ComponentMap componentMap;

        public:
            void addComponent(Plugin::Component *component, std::unique_ptr<Plugin::Component::Data> &&data)
            {
                componentMap[component->getIdentifier()] = std::move(data);
            }

            void removeComponent(const std::type_index &type)
            {
                auto componentSearch = componentMap.find(type);
                if (componentSearch != componentMap.end())
                {
                    componentMap.erase(componentSearch);
                }
            }

            // Edit::Entity
            ComponentMap &getComponentMap(void)
            {
                return componentMap;
            }

            // Plugin::Entity
            bool hasComponent(const std::type_index &type) const
            {
                return (componentMap.count(type) > 0);
            }

			Plugin::Component::Data *getComponent(const std::type_index &type)
			{
				auto componentSearch = componentMap.find(type);
				if (componentSearch == componentMap.end())
				{
					throw ComponentNotFound();
				}

				return componentSearch->second.get();
			}

			const Plugin::Component::Data *getComponent(const std::type_index &type) const
			{
				auto componentSearch = componentMap.find(type);
				if (componentSearch == componentMap.end())
				{
					throw ComponentNotFound();
				}

				return componentSearch->second.get();
			}
		};

        GEK_CONTEXT_USER(Population, Plugin::Core *)
            , public Edit::Population
        {
        private:
            Plugin::Core *core;

            float worldTime;
            float frameTime;

            std::unordered_map<String, std::type_index> componentNamesMap;
            ComponentMap componentMap;

            ThreadPool loadPool;
            std::future<bool> loaded;
            concurrency::concurrent_queue<String> loadQueue;
            concurrency::concurrent_queue<std::function<void(void)>> entityQueue;
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
                    auto componentSearch = componentMap.insert(std::make_pair(component->getIdentifier(), component));
                    if (componentSearch.second)
                    {
                        if (!componentNamesMap.insert(std::make_pair(component->getName(), component->getIdentifier())).second)
                        {
                            componentMap.erase(componentSearch.first);
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
                componentMap.clear();
            }

            bool loadLevel(const String &populationName)
            {
                try
                {
                    entityMap.clear();
                    entityQueue.clear();
                    sendShout(&Plugin::PopulationListener::onLoadBegin, populationName);
                    if (!populationName.empty())
                    {
                        Xml::Node worldNode = Xml::load(getContext()->getFileName(L"data\\scenes", populationName).append(L".xml"), L"world");

                        auto &prefabsNode = worldNode.getChild(L"prefabs");
                        for (auto &entityNode : worldNode.getChild(L"population").children)
                        {
                            std::unordered_map<String, Xml::Leaf> entityComponentMap;
                            prefabsNode.findChild(entityNode.getAttribute(L"prefab"), [&](auto &prefabNode) -> void
                            {
                                for (auto &prefabComponentNode : prefabNode.children)
                                {
                                    entityComponentMap[prefabComponentNode.type] = prefabComponentNode;
                                }
                            });

                            for (auto &componentNode : entityNode.children)
                            {
                                auto insertSearch = entityComponentMap.insert(std::make_pair(componentNode.type, componentNode));
                                if (!insertSearch.second)
                                {
                                    auto &entityComponentData = insertSearch.first->second;
                                    if (!componentNode.text.empty())
                                    {
                                        entityComponentData.text = componentNode.text;
                                    }

                                    for (auto &attribute : componentNode.attributes)
                                    {
                                        entityComponentData.attributes[attribute.first] = attribute.second;
                                    }
                                }
                            }

                            auto entity(std::make_shared<Entity>());
                            for (auto &entityComponentData : entityComponentMap)
                            {
                                addComponent(entity.get(), entityComponentData.second);
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
                    }

                    frameTime = 0.0f;
                    worldTime = 0.0f;
                    sendShout(&Plugin::PopulationListener::onLoadSucceeded, populationName);
                }
                catch (const std::exception &)
                {
                    sendShout(&Plugin::PopulationListener::onLoadFailed, populationName);
                };

                return true;
            }

            // Edit::Population
            ComponentMap &getComponentMap(void)
            {
                return componentMap;
            }

            EntityMap &getEntityMap(void)
            {
                return entityMap;
            }

            Edit::Component *getComponent(const std::type_index &type)
            {
                auto componentsSearch = componentMap.find(type);
                if (componentsSearch != componentMap.end())
                {
                    return dynamic_cast<Edit::Component *>(componentsSearch->second.get());
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
                else
                {
                    String populationName;
                    if (loadQueue.try_pop(populationName))
                    {
                        if (loaded.valid())
                        {
                            loaded.get();
                        }

                        loaded = loadPool.enqueue([this, populationName](void) -> bool
                        {
                            return loadLevel(populationName);
                        });

                        state = Plugin::PopulationStep::State::Loading;
                    }
                    else if (isBackgroundProcess)
                    {
                        state = Plugin::PopulationStep::State::Idle;
                    }
                    else
                    {
                        state = Plugin::PopulationStep::State::Active;
                        this->frameTime = frameTime;
                        this->worldTime += frameTime;
                    }
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
                loadQueue.push(populationName);
            }

            void save(const wchar_t *populationName)
            {
                GEK_REQUIRE(populationName);
            }

            Plugin::Entity *createEntity(const wchar_t *entityName, const std::vector<Xml::Leaf> &componentList)
            {
                std::shared_ptr<Entity> entity(std::make_shared<Entity>());
                for (auto &componentData : componentList)
                {
                    addComponent(entity.get(), componentData);
                }

                sendShout(&Plugin::PopulationListener::onEntityCreated, entity.get(), entityName);
                entityQueue.push([this, entityName = String(entityName), entity](void) -> void
                {
                    entityMap.insert(std::make_pair((entityName.empty() ? String::create(L"unnamed_%v", ++uniqueEntityIdentifier) : entityName), entity));
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

            bool addComponent(Entity *entity, const Xml::Leaf &componentData, std::type_index *componentIdentifier = nullptr)
            {
                GEK_REQUIRE(entity);

                auto componentNameSearch = componentNamesMap.find(componentData.type);
                if (componentNameSearch != componentNamesMap.end())
                {
                    auto componentSearch = componentMap.find(componentNameSearch->second);
                    if (componentSearch != componentMap.end())
                    {
                        Plugin::Component *componentManager = componentSearch->second.get();
                        auto component(componentManager->create());
                        componentManager->load(component.get(), componentData);

                        entity->addComponent(componentManager, std::move(component));
                        if (componentIdentifier)
                        {
                            (*componentIdentifier) = componentNameSearch->second;
                        }

                        return true;
                    }
                }

                return false;
            }

            void addComponent(Plugin::Entity *entity, const Xml::Leaf &componentData)
            {
                std::type_index componentIdentifier(typeid(nullptr));
                if (addComponent(static_cast<Entity *>(entity), componentData, &componentIdentifier))
                {
                    sendShout(&Plugin::PopulationListener::onComponentAdded, static_cast<Plugin::Entity *>(entity), componentIdentifier);
                }
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
