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
    interface Entity;
    interface PopulationObserver;

    interface Population
    {
        struct ComponentDefinition : public CStringW, public std::unordered_map<CStringW, CStringW>
        {
        };

        struct EntityDefinition : public std::unordered_map<CStringW, ComponentDefinition>
        {
        };

        void initialize(IUnknown *initializerContext);
        void destroy(void);

        float getWorldTime(void);
        float getFrameTime(void);

        void update(bool isIdle, float frameTime = 0.0f);

        void load(LPCWSTR fileName);
        void save(LPCWSTR fileName);
        void free(void);

        Entity *createEntity(const EntityDefinition &entityParameterList, LPCWSTR name = nullptr);
        void killEntity(Entity *entity);
        Entity *getNamedEntity(LPCWSTR name);

        void listEntities(std::function<void(Entity *)> onEntity);

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

        void listProcessors(std::function<void(Processor *)> onProcessor);

        UINT32 setUpdatePriority(PopulationObserver *observer, UINT32 priority);
        void removeUpdatePriority(UINT32 updateHandle);
    };

    DECLARE_INTERFACE_IID(PopulationObserver, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9") : virtual public Observer
    {
        void onLoadBegin(void) = default;
        void onLoadEnd(HRESULT resultValue) = default;
        void onFree(void) = default;

        void onEntityCreated(Entity *entity) = default;
        void onEntityDestroyed(Entity *entity) = default;

        void onUpdate(UINT32 handle, bool isIdle) = default;
    };

    DECLARE_INTERFACE_IID(PopulationRegistration, "BD97404A-DE56-4DDC-BB34-3190FD51DEE5");
}; // namespace Gek