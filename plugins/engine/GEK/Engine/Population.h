#pragma once

#include "GEK\Utility\Hash.h"
#include "GEK\Context\Observer.h"
#include "GEK\Engine\Processor.h"
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Gek
{
    GEK_PREDECLARE(Entity);
    GEK_PREDECLARE(PopulationObserver);

    GEK_INTERFACE(Population)
    {
        struct ComponentDefinition
            : public CStringW, public std::unordered_map<CStringW, CStringW>
        {
        };

        struct EntityDefinition
            : public std::unordered_map<CStringW, ComponentDefinition>
        {
        };

        virtual void initialize(IUnknown *initializerContext) = 0;
        virtual void destroy(void) = 0;

        virtual float getWorldTime(void) = 0;
        virtual float getFrameTime(void) = 0;

        virtual void update(bool isIdle, float frameTime = 0.0f) = 0;

        virtual void load(const wchar_t *fileName) = 0;
        virtual void save(const wchar_t *fileName) = 0;
        virtual void free(void) = 0;

        virtual Entity *createEntity(const EntityDefinition &entityParameterList, const wchar_t *name = nullptr) = 0;
        virtual void killEntity(Entity *entity) = 0;
        virtual Entity *getNamedEntity(const wchar_t *name) = 0;

        virtual void listEntities(std::function<void(Entity *)> onEntity) = 0;

        template<typename... ARGUMENTS>
        void listEntities(std::function<void(Entity *)> onEntity)
        {
            listEntities([onEntity](Entity *entity) -> void
            {
                if (entity->hasComponents<ARGUMENTS...>())
                {
                    onEntity(entity);
                }
            });
        }

        virtual void listProcessors(std::function<void(Processor *)> onProcessor) = 0;

        virtual UINT32 setUpdatePriority(PopulationObserver *observer, UINT32 priority) = 0;
        virtual void removeUpdatePriority(UINT32 updateHandle) = 0;
    };

    GEK_INTERFACE(PopulationObserver)
    {
        virtual void onLoadBegin(void) = 0;
        virtual void onLoadEnd(HRESULT resultValue) = 0;
        virtual void onFree(void) = 0;

        virtual void onEntityCreated(Entity *entity) = 0;
        virtual void onEntityDestroyed(Entity *entity) = 0;

        virtual void onUpdate(UINT32 handle, bool isIdle) = 0;
    };
}; // namespace Gek