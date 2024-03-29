﻿#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Population.hpp"
#include <tbb/concurrent_queue.h>
#include <execution>
#include <unordered_map>
#include <vector>
#include <map>

namespace Gek
{
    namespace Implementation
    {
        class Entity
            : public Edit::Entity
        {
        private:
            Components components;

        public:
            void addComponent(Plugin::Component *component, std::unique_ptr<Plugin::Component::Data> &&data)
            {
                components[component->getIdentifier()] = std::move(data);
            }

            void removeComponent(Hash type)
            {
                auto componentSearch = components.find(type);
                if (componentSearch != std::end(components))
                {
                    components.erase(componentSearch);
                }
            }

            void listComponents(std::function<void(Hash , Plugin::Component::Data const *)> onComponent)
            {
                for (auto const &component : components)
                {
                    onComponent(component.first, component.second.get());
                }
            }

            // Edit::Entity
            Components &getComponents(void)
            {
                return components;
            }

            // Plugin::Entity
            bool hasComponent(Hash type) const
            {
                return (components.count(type) > 0);
            }

			Plugin::Component::Data *getComponent(Hash type)
			{
				auto componentSearch = components.find(type);
				if (componentSearch == std::end(components))
				{
                    return nullptr;
				}

				return componentSearch->second.get();
			}

			const Plugin::Component::Data *getComponent(Hash type) const
			{
				auto componentSearch = components.find(type);
				if (componentSearch == std::end(components))
				{
                    return nullptr;
                }

				return componentSearch->second.get();
			}
		};

        GEK_CONTEXT_USER(Population, Engine::Core *)
            , public Engine::Population
        {
        private:
            Engine::Core *core = nullptr;

            ShuntingYard shuntingYard;
            tbb::concurrent_queue<Action> actionQueue;

            std::unordered_map<std::string, Hash> componentTypeNameMap;
            std::unordered_map<Hash, std::string> componentNameTypeMap;
            AvailableComponents availableComponents;

            ThreadPool workerPool;
            tbb::concurrent_queue<std::function<void(void)>> entityQueue;
            Registry registry;

            uint32_t uniqueEntityIdentifier = 0;

        public:
            Population(Context *context, Engine::Core *core)
                : ContextRegistration(context)
                , core(core)
                , workerPool(1)
            {
                assert(core);

                getContext()->log(Context::Info, "Loading component plugins");
                getContext()->listTypes("ComponentType", [&](std::string_view className) -> void
                {
                    getContext()->log(Context::Info, "Component found: {}", className);
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className, static_cast<Plugin::Population *>(this)));
                    if (availableComponents.count(component->getIdentifier()) > 0)
                    {
                        std::cerr << "Duplicate component identifier found: Class(" << className << "), Identifier(" << component->getIdentifier() << ")";
                        return;
                    }

                    if (componentNameTypeMap.count(component->getIdentifier()) > 0)
                    {
                        std::cerr << "Duplicate component name found: Class(" << className << "), Name(" << component->getName() << ")";
                        return;
                    }

                    componentNameTypeMap.insert(std::make_pair(component->getIdentifier(), component->getName()));
                    componentTypeNameMap.insert(std::make_pair(component->getName(), component->getIdentifier()));
                    availableComponents[component->getIdentifier()] = std::move(component);
                });

                core->onShutdown.connect(this, &Population::onShutdown);
            }

            ~Population(void)
            {
                workerPool.drain();
                componentTypeNameMap.clear();
                availableComponents.clear();
            }

            void queueEntity(Plugin::Entity *entity)
            {
                entityQueue.push([this, entity](void) -> void
                {
                    registry.push_back(Plugin::EntityPtr(entity));
                    onEntityCreated(entity);
                });
            }

            // Core
            void onShutdown(void)
            {
                workerPool.reset();
                registry.clear();
            }

            // Edit::Population
            AvailableComponents &getAvailableComponents(void)
            {
                return availableComponents;
            }

            Registry &getRegistry(void)
            {
                return registry;
            }

            Edit::Component *getComponent(Hash type)
            {
                auto componentsSearch = availableComponents.find(type);
                if (componentsSearch != std::end(availableComponents))
                {
                    return dynamic_cast<Edit::Component *>(componentsSearch->second.get());
                }
                
                return nullptr;
            }

            // Plugin::Population
            ShuntingYard &getShuntingYard(void)
            {
                return shuntingYard;
            }

            void update(float frameTime)
            {
				if (frameTime == 0.0f)
				{
					actionQueue.clear();
				}
				else
				{
					Action action;
					while (actionQueue.try_pop(action))
					{
						onAction(action);
					};
				}

				for (auto &slot : onUpdate)
				{
					slot.second(frameTime);
				}

				std::function<void(void)> entityAction;
				while (entityQueue.try_pop(entityAction))
				{
					entityAction();
				};
            }

            void action(Action const &action)
            {
                actionQueue.push(action);
            }

            void fullReset(void)
            {
                actionQueue.clear();
                onReset();
                registry.clear();
            }

            Task scheduleReset(void)
            {
                co_await workerPool.schedule();
                fullReset();
            }

            void reset(void)
            {
                scheduleReset();
            }

            Task scheduleLoad(std::string const& _populationName)
            {
                std::string populationName(_populationName);
                fullReset();
                workerPool.join();

                co_await workerPool.schedule();

                getContext()->log(Context::Info, "Loading population: {}", populationName);

                JSON::Object worldNode = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("scenes", populationName).withExtension(".json")));
                shuntingYard.setRandomSeed(JSON::Value(worldNode, "Seed", uint32_t(std::time(nullptr) & 0xFFFFFFFF)));

                auto templatesNode = worldNode["Templates"];
                auto populationNode = worldNode["Population"];
                getContext()->log(Context::Info, "Found {} Entity Definitions", populationNode.size());
                for (auto &entityNode : populationNode)
                {
                    uint32_t count = 1;
                    EntityDefinition entityDefinition;
                    auto templateSearch = entityNode.find("Template");
                    if (templateSearch != std::end(entityNode))
                    {
                        std::string templateName;
                        auto entityTemplateNode = entityNode["Template"];
                        if (entityTemplateNode.is_string())
                        {
                            templateName = entityTemplateNode.get<std::string>();
                        }
                        else
                        {
                            if (entityTemplateNode.contains("Base"))
                            {
                                templateName = JSON::Value(entityTemplateNode, "Base", String::Empty);
                            }

                            if (entityTemplateNode.contains("Count"))
                            {
                                count = JSON::Value(entityTemplateNode, "Count", 0);
                            }
                        }

                        auto& templateNode = templatesNode[templateName];
                        for (auto const& [key, value] : templateNode.items())
                        {
                            entityDefinition[key] = value;
                        }

                        entityNode.erase(templateSearch);
                    }

                    for (auto const& [componentName, componentData] : entityNode.items())
                    {
                        auto& componentDefinition = entityDefinition[componentName];
                        if (componentData.is_object())
                        {
                            for (auto const& [valueName, valueData] : componentData.items())
                            {
                                componentDefinition[valueName] = valueData;
                            }
                        }
                        else
                        {
                            componentDefinition = componentData;
                        }
                    }

                    while (count-- > 0)
                    {
                        auto populationEntity = new Entity();
                        for (auto const& componentDefiniti9on : entityDefinition)
                        {
                            addComponent(populationEntity, componentDefiniti9on);
                        }

                        auto entity = dynamic_cast<Plugin::Entity*>(populationEntity);
                        queueEntity(entity);
                    };
                }

                onLoad.emit(populationName);
            }

            void load(std::string const& populationName)
            {
                scheduleLoad(populationName);
            }

            void save(std::string const &populationName)
            {
                auto population = JSON::Object({});
                for (auto const &entity : registry)
                {
                    JSON::Object entityDefinition;
                    Entity *editorEntity = static_cast<Entity *>(entity.get());
                    editorEntity->listComponents([&](Hash type, Plugin::Component::Data const *data) -> void
                    {
                        auto componentName = componentNameTypeMap.find(type);
                        if (componentName == std::end(componentNameTypeMap))
                        {
                            std::cerr << "Unknown component identifier found when trying to save population: " << type;
                        }
                        else
                        {
                            auto component = availableComponents.find(type);
                            if (component == std::end(availableComponents))
                            {
								std::cerr << "Unknown component type found when trying to save population: " << componentName->second << ", " << type;
                            }
                            else
                            {
                                JSON::Object componentDefinition;
                                component->second->save(data, componentDefinition);
                                entityDefinition[componentName->second] = componentDefinition;
                            }
                        }
                    });

                    population.push_back(entityDefinition);
                }

                JSON::Object scene;
                scene["Population"] = population;
                scene["Seed"] = shuntingYard.getRandomSeed();
                JSON::Save(scene, getContext()->getCachePath(FileSystem::CreatePath("scenes", populationName).withExtension(".json")));
                onSave.emit(populationName);
            }

            Plugin::Entity *createEntity(EntityDefinition const &entityDefinition)
            {
                auto populationEntity = new Entity();
                for (auto const &componentDefinition : entityDefinition)
                {
                    addComponent(populationEntity, componentDefinition);
                }

                auto entity = dynamic_cast<Plugin::Entity *>(populationEntity);
                queueEntity(entity);
                return entity;
            }

            void killEntity(Plugin::Entity * const entity)
            {
                entityQueue.push([this, entity](void) -> void
                {
                    auto entitySearch = std::find_if(std::begin(registry), std::end(registry), [entity](auto const &entitySearch) -> bool
                    {
                        return (entitySearch.get() == entity);
                    });

                    if (entitySearch != std::end(registry))
                    {
                        onEntityDestroyed(entity);
                        registry.erase(entitySearch);
                    }
                });
            }

            bool addComponent(Entity *entity, ComponentDefinition const &definition)
            {
                assert(entity);

				auto componentNameSearch = std::find_if(std::begin(componentTypeNameMap), std::end(componentTypeNameMap), [&definition](auto const &componentPair) -> bool
				{
					return (definition.first == componentPair.first);
				});

                if (componentNameSearch != std::end(componentTypeNameMap))
                {
                    auto componentSearch = availableComponents.find(componentNameSearch->second);
                    if (componentSearch != std::end(availableComponents))
                    {
                        Plugin::Component *componentManager = componentSearch->second.get();
                        auto component(componentManager->create());
                        componentManager->load(component.get(), definition.second);

                        entity->addComponent(componentManager, std::move(component));
                        return true;
                    }
                    else
                    {
                        std::cerr << "Entity contains unknown component identifier: " << definition.first << ", " << componentNameSearch->second;
                    }
                }
                else
                {
                    std::cerr << "Entity contains unknown component: " << definition.first;
                }

                return false;
            }

            void addComponent(Plugin::Entity * const entity, ComponentDefinition const &definition)
            {
                if (addComponent(static_cast<Entity *>(entity), definition))
                {
                    onComponentAdded(static_cast<Plugin::Entity *>(entity));
                }
            }

            void removeComponent(Plugin::Entity * const entity, Hash type)
            {
                assert(entity);

                if (entity->hasComponent(type))
                {
                    onComponentRemoved(entity);
                    static_cast<Entity *>(entity)->removeComponent(type);
                }
            }

            void listEntities(std::function<void(Plugin::Entity *)> &&onEntity) const
            {
                std::for_each(std::execution::par, std::begin(registry), std::end(registry), [onEntity = std::move(onEntity)](auto &entity) -> void
                {
                    onEntity(entity.get());
                });
            }
        };

        GEK_REGISTER_CONTEXT_USER(Population);
    }; // namespace Implementation
}; // namespace Gek
