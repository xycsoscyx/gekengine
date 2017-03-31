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
#include <nano_signal_slot.hpp>
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
            GEK_ADD_EXCEPTION(FatalError);
            GEK_ADD_EXCEPTION(InvalidTemplatesBlock);
            GEK_ADD_EXCEPTION(InvalidPopulationBlock);
            GEK_ADD_EXCEPTION(InvalidEntityBlock);
            GEK_ADD_EXCEPTION(EntityNameExists);

            struct Action
            {
                WString name;
                union
                {
                    bool state;
                    float value;
                };

                Action(void)
                {
                }

                Action(wchar_t const * name, bool state)
                    : name(name)
                    , state(state)
                {
                }

                Action(wchar_t const * name, float value)
                    : name(name)
                    , value(value)
                {
                }
            };

            virtual ~Population(void) = default;

            std::map<int32_t, Nano::Signal<void(float frameTime)>> onUpdate;
            Nano::Signal<void(Action const &action)> onAction;

            Nano::Signal<void(Plugin::Entity * const entity, wchar_t const * const entityName)> onEntityCreated;
            Nano::Signal<void(Plugin::Entity * const entity)> onEntityDestroyed;

            Nano::Signal<void(Plugin::Entity * const entity, const std::type_index &type)> onComponentAdded;
            Nano::Signal<void(Plugin::Entity * const entity, const std::type_index &type)> onComponentRemoved;

            virtual ShuntingYard &getShuntingYard(void) = 0;

            virtual void load(wchar_t const * const populationName) = 0;
            virtual void save(wchar_t const * const populationName) = 0;

            virtual Plugin::Entity *createEntity(wchar_t const * const entityName, const std::vector<JSON::Member> &componentList = std::vector<JSON::Member>()) = 0;
            virtual void killEntity(Plugin::Entity * const entity) = 0;
            virtual void addComponent(Plugin::Entity * const entity, const JSON::Member &componentData) = 0;
            virtual void removeComponent(Plugin::Entity * const entity, const std::type_index &type) = 0;

            virtual void listEntities(std::function<void(Plugin::Entity * const entity, wchar_t const * const entityName)> onEntity) const = 0;

            template<typename... COMPONENTS>
            void listEntities(std::function<void(Plugin::Entity * const entity, wchar_t const * const entityName, COMPONENTS&... components)> onEntity) const
            {
                listEntities([onEntity = move(onEntity)](Plugin::Entity * const entity, wchar_t const * const entityName) -> void
                {
                    if (entity->hasComponents<COMPONENTS...>())
                    {
                        onEntity(entity, entityName, entity->getComponent<COMPONENTS>()...);
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
            using ComponentMap = std::unordered_map<std::type_index, Plugin::ComponentPtr>;
            virtual ComponentMap &getComponentMap(void) = 0;

            using EntityMap = std::unordered_map<WString, Plugin::EntityPtr>;
            virtual EntityMap &getEntityMap(void) = 0;

            virtual ~Population(void) = default;

            virtual Edit::Component *getComponent(const std::type_index &type) = 0;
        };
    }; // namespace Edit
}; // namespace Gek