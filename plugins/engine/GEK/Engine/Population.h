#pragma once

#include "GEK\Utility\String.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\Observable.h"
#include "GEK\Engine\Processor.h"
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);
        GEK_PREDECLARE(PopulationObserver);

        GEK_INTERFACE(Population)
            : virtual public Observable
        {
            GEK_START_EXCEPTIONS();

            struct ComponentDefinition
                : public std::unordered_map<String, String>
            {
                String value;
            };

            struct EntityDefinition
                : public std::unordered_map<String, ComponentDefinition>
            {
            };

            virtual float getWorldTime(void) const = 0;
            virtual float getFrameTime(void) const = 0;

            virtual void update(bool isIdle, float frameTime = 0.0f) = 0;

            virtual void load(const wchar_t *populationName) = 0;
            virtual void save(const wchar_t *populationName) = 0;
            virtual void free(void) = 0;

            virtual Plugin::Entity *createEntity(const EntityDefinition &entityParameterList, const wchar_t *entityName = nullptr) = 0;
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

            virtual void listProcessors(std::function<void(Processor *)> onProcessor) const = 0;

            virtual uint32_t setUpdatePriority(PopulationObserver *observer, uint32_t priority) = 0;
            virtual void removeUpdatePriority(uint32_t updateHandle) = 0;
        };

        GEK_INTERFACE(PopulationObserver)
            : public Observer
        {
            virtual void onLoadBegin(void) { };
            virtual void onLoadSucceeded(void) { };
            virtual void onLoadFailed(void) { };
            virtual void onFree(void) { };

            virtual void onEntityCreated(Plugin::Entity *entity) { };
            virtual void onEntityDestroyed(Plugin::Entity *entity) { };

            virtual void onUpdate(uint32_t handle, bool isIdle) { };
        };
    }; // namespace Plugin

    namespace Engine
    {
        GEK_INTERFACE(Population)
            : virtual public Gek::Plugin::Population
        {
            virtual void loadComponents(void) = 0;
            virtual void freeComponents(void) = 0;
        };
    }; // namespace Engine
}; // namespace Gek