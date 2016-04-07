#pragma once

#include "GEK\Context\Observer.h"
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Gek
{
    DECLARE_INTERFACE(Entity);
    DECLARE_INTERFACE(PopulationObserver);

    DECLARE_INTERFACE_IID(Population, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B") : virtual public IUnknown
    {
        STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext) PURE;

        STDMETHOD_(float, getWorldTime)             (THIS) PURE;
        STDMETHOD_(float, getFrameTime)             (THIS) PURE;

        STDMETHOD_(void, update)                    (THIS_ bool isIdle, float frameTime = 0.0f) PURE;

        STDMETHOD(load)                             (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD(save)                             (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(void, free)                      (THIS) PURE;

        STDMETHOD_(Entity *, createEntity)          (THIS_ const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &entityParameterList, LPCWSTR name = nullptr) PURE;
        STDMETHOD_(void, killEntity)                (THIS_ Entity *entity) PURE;
        STDMETHOD_(Entity *, getNamedEntity)        (THIS_ LPCWSTR name) PURE;

        STDMETHOD_(void, listEntities)              (THIS_ std::function<void(Entity *)> onEntity, bool runInParallel = false) PURE;

        template<typename... ARGS>
        void listEntities(std::function<void(Entity *)> onEntity)
        {
            listEntities([&](Entity *entity) -> void
            {
                if (entity->hasComponents<ARGS...>())
                {
                    onEntity(entity);
                }
            });
        }

        STDMETHOD_(UINT32, setUpdatePriority)       (THIS_ PopulationObserver *observer, UINT32 priority) PURE;
        STDMETHOD_(void, removeUpdatePriority)      (THIS_ UINT32 updateHandle) PURE;
    };

    DECLARE_INTERFACE_IID(PopulationObserver, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9") : virtual public Observer
    {
        STDMETHOD_(void, onLoadBegin)               (THIS) { };
        STDMETHOD_(void, onLoadEnd)                 (THIS_ HRESULT resultValue) { };
        STDMETHOD_(void, onFree)                    (THIS) { };

        STDMETHOD_(void, onEntityCreated)           (THIS_ Entity *entity) { };
        STDMETHOD_(void, onEntityDestroyed)         (THIS_ Entity *entity) { };

        STDMETHOD_(void, onUpdate)                  (THIS_ bool isIdle) { };
    };

    DECLARE_INTERFACE_IID(PopulationRegistration, "BD97404A-DE56-4DDC-BB34-3190FD51DEE5");
}; // namespace Gek