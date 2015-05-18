#pragma once

#include "GEK\Utility\Common.h"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <concurrent_queue.h>
#include <unordered_map>
#include <set>

#pragma warning(disable:4503)

namespace Gek
{
    namespace Engine
    {
        namespace Component
        {
            DECLARE_INTERFACE_IID(Type, "7B71BEF1-A2B2-4791-A664-2EA631E04392");

            DECLARE_INTERFACE_IID(Interface, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E") : virtual public IUnknown
            {
                STDMETHOD_(LPCWSTR, getName)                (THIS) const PURE;
                STDMETHOD_(Handle, getIdentifier)           (THIS) const PURE;

                STDMETHOD_(void, addComponent)              (THIS_ Handle entityHandle) PURE;
                STDMETHOD_(void, removeComponent)           (THIS_ Handle entityHandle) PURE;
                STDMETHOD_(bool, hasComponent)              (THIS_ Handle entityHandle) const PURE;
                STDMETHOD_(LPVOID, getComponent)            (THIS_ Handle entityHandle) PURE;
                STDMETHOD_(void, clear)                     (THIS)PURE;

                STDMETHOD_(void, getIntersectingSet)        (THIS_ std::set<Handle> &entityList) PURE;

                STDMETHOD(getData)                          (THIS_ Handle entityHandle, std::unordered_map<CStringW, CStringW> &componentParameterList) PURE;
                STDMETHOD(setData)                          (THIS_ Handle entityHandle, const std::unordered_map<CStringW, CStringW> &componentParameterList) PURE;

                template <typename CLASS>
                CLASS &getComponent(Handle entityHandle)
                {
                    return *(CLASS *)getComponent(nEntityID);
                }
            };
        }; // namespace Component
    }; // namespace Engine
}; // namespace Gek
