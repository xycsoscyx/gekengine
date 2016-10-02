#pragma once

#include "GEK\Utility\String.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\Broadcaster.h"
#include "GEK\Engine\Processor.h"
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <vector>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        GEK_INTERFACE(PopulationListener)
        {
            virtual void onLoadBegin(void) { };
            virtual void onLoadSucceeded(void) { };
            virtual void onLoadFailed(void) { };

            virtual void onEntityCreated(Plugin::Entity *entity) { };
            virtual void onEntityDestroyed(Plugin::Entity *entity) { };
        };

        GEK_INTERFACE(PopulationStep)
        {
            enum class State : uint8_t
            {
                Unknown = 0,
                Active,
                Idle,
                Loading,
            };

            virtual void onUpdate(uint32_t order, State state) = 0;
        };

        GEK_INTERFACE(Population)
            : public Broadcaster<PopulationListener>
            , public Sequencer<PopulationStep>
        {
            GEK_START_EXCEPTIONS();

            struct ComponentDefinition
                : public std::unordered_map<String, String>
            {
                String name;
                String value;
            };

            struct EntityDefinition
                : public std::list<ComponentDefinition>
            {
                String name;
            };

            virtual float getWorldTime(void) const = 0;
            virtual float getFrameTime(void) const = 0;

            virtual void update(bool isBackgroundProcess, float frameTime = 0.0f) = 0;

            virtual void load(const wchar_t *populationName) = 0;
            virtual void save(const wchar_t *populationName) = 0;

            virtual Plugin::Entity *createEntity(const EntityDefinition &entityDefinition) = 0;
            virtual void killEntity(Plugin::Entity *entity) = 0;
            virtual Plugin::Entity *getNamedEntity(const wchar_t *entityName) const = 0;

            virtual void listEntities(std::function<void(Plugin::Entity *)> onEntity) const = 0;

            template<typename... ARGUMENTS>
            void listEntities(std::function<void(Plugin::Entity *)> onEntity) const
            {
                listEntities([onEntity = move(onEntity)](Plugin::Entity *entity) -> void
                {
                    if (entity->hasComponents<ARGUMENTS...>())
                    {
                        onEntity(entity);
                    }
                });
            }
        };
    }; // namespace Plugin

    namespace Debug
    {
        GEK_INTERFACE(Population)
            : public Plugin::Population
        {
            virtual std::vector<Plugin::EntityPtr> &getEntityList(void) = 0;
        };
    };
}; // namespace Gek