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
    // {7B71BEF1-A2B2-4791-A664-2EA631E04392}
    DEFINE_GUID(ComponentType, 0x7b71bef1, 0xa2b2, 0x4791, 0xa6, 0x64, 0x2e, 0xa6, 0x31, 0xe0, 0x43, 0x92);

    template <typename CONVERSION, typename TYPE>
    void setComponentParameter(const std::unordered_map<CStringW, CStringW> &list, LPCWSTR name, TYPE &value, CONVERSION convert)
    {
        auto iterator = list.find(name);
        if (iterator != list.end())
        {
            value = convert((*iterator).second);
        }
    }

    DECLARE_INTERFACE_IID_(ComponentInterface, IUnknown, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E")
    {
        STDMETHOD_(LPCWSTR, getName)                (THIS) const PURE;
        STDMETHOD_(Handle, getIdentifier)           (THIS) const PURE;

        STDMETHOD_(void, addComponent)              (THIS_ Handle entityHandle) PURE;
        STDMETHOD_(void, removeComponent)           (THIS_ Handle entityHandle) PURE;
        STDMETHOD_(bool, hasComponent)              (THIS_ Handle entityHandle) const PURE;
        STDMETHOD_(LPVOID, getComponent)            (THIS_ Handle entityHandle) PURE;
        STDMETHOD_(void, clear)                     (THIS) PURE;

        STDMETHOD_(void, getIntersectingSet)        (THIS_ std::set<Handle> &entityList) PURE;

        STDMETHOD(getData)                          (THIS_ Handle entityHandle, std::unordered_map<CStringW, CStringW> &componentParameterList) PURE;
        STDMETHOD(setData)                          (THIS_ Handle entityHandle, const std::unordered_map<CStringW, CStringW> &componentParameterList) PURE;

        template <typename CLASS>
        CLASS &getComponent(Handle entityHandle)
        {
            return *(CLASS *)getComponent(nEntityID);
        }
    };
}; // namespace Gek
