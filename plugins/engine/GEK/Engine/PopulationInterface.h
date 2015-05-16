#pragma once

#include "GEK\Utility\Common.h"
#include "GEK\Context\ObserverInterface.h"
#include <unordered_map>
#include <vector>
#include <functional>

namespace Gek
{
    // {BD97404A-DE56-4DDC-BB34-3190FD51DEE5}
    DEFINE_GUID(PopulationSystem, 0xbd97404a, 0xde56, 0x4ddc, 0xbb, 0x34, 0x31, 0x90, 0xfd, 0x51, 0xde, 0xe5);

    namespace Population
    {
        DECLARE_INTERFACE_IID_(SystemInterface, IUnknown, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B")
        {
            STDMETHOD(initialize)                       (THIS) PURE;

            STDMETHOD_(float, getGameTime)              (THIS) const PURE;

            STDMETHOD(load)                             (THIS_ LPCWSTR fileName) PURE;
            STDMETHOD(save)                             (THIS_ LPCWSTR fileName) PURE;
            STDMETHOD_(void, free)                      (THIS) PURE;

            STDMETHOD_(Handle, createEntity)            (THIS_ const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &entityParameterList, LPCWSTR name = nullptr) PURE;
            STDMETHOD_(void, killEntity)                (THIS_ Handle entityHandle) PURE;
            STDMETHOD_(Handle, getNamedEntity)          (THIS_ LPCWSTR name) PURE;

            STDMETHOD_(void, listEntities)              (THIS_ std::function<void(Handle)> onEntity, bool runInParallel = false) PURE;
            STDMETHOD_(void, listEntities)              (THIS_ const std::vector<Handle> &requiredComponentList, std::function<void(Handle)> onEntity, bool runInParallel = false) PURE;

            STDMETHOD_(bool, hasComponent)              (THIS_ Handle entityHandle, Handle componentHandle) PURE;
            STDMETHOD_(LPVOID, getComponent)            (THIS_ Handle entityHandle, Handle componentHandle) PURE;

            template <typename CLASS>
            CLASS &getComponent(Handle entityHandle, Handle componentHandle)
            {
                return *(CLASS *)getComponent(entityHandle, componentHandle);
            }
        };

        DECLARE_INTERFACE_IID_(ObserverInterface, Gek::ObserverInterface, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9")
        {
            STDMETHOD_(void, onLoadBegin)               (THIS) { };
            STDMETHOD_(void, onLoadEnd)                 (THIS_ HRESULT resultValue) { };
            STDMETHOD_(void, onFree)                    (THIS) { };

            STDMETHOD_(void, onEntityCreated)           (THIS_ Handle entityHandle) { };
            STDMETHOD_(void, onEntityDestroyed)         (THIS_ Handle entityHandle) { };

            STDMETHOD_(void, onUpdateBegin)             (THIS_ float frameTime) { };
            STDMETHOD_(void, onUpdate)                  (THIS_ float frameTime) { };
            STDMETHOD_(void, onUpdateEnd)               (THIS_ float frameTime) { };
        };
    }; // namespace Population
}; // namespace Gek