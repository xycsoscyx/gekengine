#pragma once

#include "GEK\Utility\String.hpp"
#include "GEK\Utility\Context.hpp"
#include "GEK\Engine\Processor.hpp"
#include <nano_signal_slot.hpp>
#include <unordered_map>
#include <functional>
#include <typeindex>
#include <vector>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);
        GEK_PREDECLARE(Component);

        GEK_INTERFACE(Population)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(EntityNameExists);

            enum class State : uint8_t
            {
                Unknown = 0,
                Active,
                Idle,
                Loading,
            };

            std::map<int32_t, Nano::Signal<void(State state)>> onUpdate;
            Nano::Signal<void(const String &populationName)> onLoadBegin;
            Nano::Signal<void(const String &populationName)> onLoadSucceeded;
            Nano::Signal<void(const String &populationName)> onLoadFailed;

            Nano::Signal<void(Plugin::Entity *entity, const wchar_t *entityName)> onEntityCreated;
            Nano::Signal<void(Plugin::Entity *entity)> onEntityDestroyed;

            Nano::Signal<void(Plugin::Entity *entity, const std::type_index &type)> onComponentAdded;
            Nano::Signal<void(Plugin::Entity *entity, const std::type_index &type)> onComponentRemoved;

            virtual float getWorldTime(void) const = 0;
            virtual float getFrameTime(void) const = 0;

            virtual void load(const wchar_t *populationName) = 0;
            virtual void save(const wchar_t *populationName) = 0;

            virtual Plugin::Entity *createEntity(const wchar_t *entityName, const std::vector<Xml::Leaf> &componentList = std::vector<Xml::Leaf>()) = 0;
            virtual void killEntity(Plugin::Entity *entity) = 0;
            virtual void addComponent(Plugin::Entity *entity, const Xml::Leaf &componentData) = 0;
            virtual void removeComponent(Plugin::Entity *entity, const std::type_index &type) = 0;

            virtual void listEntities(std::function<void(Plugin::Entity *entity, const wchar_t *entityName)> onEntity) const = 0;

            template<typename... PARAMETERS>
            void listEntities(std::function<void(Plugin::Entity *entity, const wchar_t *entityName)> onEntity) const
            {
                listEntities([onEntity = move(onEntity)](Plugin::Entity *entity, const wchar_t *entityName) -> void
                {
                    if (entity->hasComponents<PARAMETERS...>())
                    {
                        onEntity(entity, entityName);
                    }
                });
            }

            virtual void update(bool isBackgroundProcess, float frameTime = 0.0f) = 0;
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

            using EntityMap = std::unordered_map<String, Plugin::EntityPtr>;
            virtual EntityMap &getEntityMap(void) = 0;

            virtual Edit::Component *getComponent(const std::type_index &type) = 0;
        };
    }; // namespace Edit
}; // namespace Gek