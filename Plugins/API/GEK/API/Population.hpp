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
#include "GEK/API/Entity.hpp"
#include <wink/signal.hpp>
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
            using Component = std::pair<std::string, JSON::Object>;

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

            std::map<int32_t, wink::signal<wink::slot<void(float frameTime)>>> onUpdate;
            wink::signal<wink::slot<void(Action const &action)>> onAction;

            wink::signal<wink::slot<void(void)>> onReset;

            wink::signal<wink::slot<void(Plugin::Entity * const entity)>> onEntityCreated;
            wink::signal<wink::slot<void(Plugin::Entity * const entity)>> onEntityDestroyed;

            wink::signal<wink::slot<void(Plugin::Entity * const entity)>> onComponentAdded;
            wink::signal<wink::slot<void(Plugin::Entity * const entity)>> onComponentRemoved;

            virtual ShuntingYard &getShuntingYard(void) = 0;

            virtual void load(std::string const &populationName) = 0;
            virtual void save(std::string const &populationName) = 0;

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

            virtual void action(Action const &action) = 0;
        };
    }; // namespace Plugin
}; // namespace Gek