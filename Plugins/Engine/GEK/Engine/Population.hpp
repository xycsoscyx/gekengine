/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Engine/Processor.hpp"
#include <lsignal.h>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <vector>
#include <map>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);
        GEK_PREDECLARE(Component);

        GEK_INTERFACE(Population)
        {
            struct Action
            {
                std::string name;
                union
                {
                    bool state;
                    float value;
                };

                Action(void)
                {
                }

                Action(std::string const &name, bool state)
                    : name(String::GetLower(name))
                    , state(state)
                {
                }

                Action(std::string const &name, float value)
					: name(String::GetLower(name))
					, value(value)
                {
                }
            };

            virtual ~Population(void) = default;

            std::map<int32_t, lsignal::signal<void(float frameTime)>> onUpdate;
            lsignal::signal<void(Action const &action)> onAction;

            lsignal::signal<void(void)> onReset;

            lsignal::signal<void(Plugin::Entity * const entity)> onEntityCreated;
            lsignal::signal<void(Plugin::Entity * const entity)> onEntityDestroyed;

            lsignal::signal<void(Plugin::Entity * const entity)> onComponentAdded;
            lsignal::signal<void(Plugin::Entity * const entity)> onComponentRemoved;

            virtual ShuntingYard &getShuntingYard(void) = 0;

            virtual void reset(void) = 0;
            virtual void load(std::string const &populationName) = 0;
            virtual void save(std::string const &populationName) = 0;

            using Component = std::pair<std::string, JSON::Object>;
            virtual Plugin::Entity *createEntity(const std::vector<Component> &componentList = std::vector<Component>()) = 0;
            virtual void killEntity(Plugin::Entity * const entity) = 0;
            virtual void addComponent(Plugin::Entity * const entity, Component const &componentData) = 0;
            virtual void removeComponent(Plugin::Entity * const entity, std::type_index const &type) = 0;

            virtual void listEntities(std::function<void(Plugin::Entity * const entity)> onEntity) const = 0;

            template<typename... COMPONENTS>
            void listEntities(std::function<void(Plugin::Entity * const entity, COMPONENTS&... components)> onEntity) const
            {
                listEntities([onEntity = move(onEntity)](Plugin::Entity * const entity) -> void
                {
                    if (entity->hasComponents<COMPONENTS...>())
                    {
                        onEntity(entity, entity->getComponent<COMPONENTS>()...);
                    }
                });
            }

            virtual void update(float frameTime = 0.0f) = 0;
            virtual void action(Action const &action) = 0;
        };
    }; // namespace Plugin

    namespace Edit
    {
        GEK_PREDECLARE(Component);

        GEK_INTERFACE(Population)
            : public Plugin::Population
        {
            virtual ~Population(void) = default;

            using ComponentMap = std::unordered_map<std::type_index, Plugin::ComponentPtr>;
            virtual ComponentMap &getComponentMap(void) = 0;

            using EntityList = std::list<Plugin::EntityPtr>;
            virtual EntityList &getEntityList(void) = 0;

            virtual Edit::Component *getComponent(const std::type_index &type) = 0;
        };
    }; // namespace Edit
}; // namespace Gek