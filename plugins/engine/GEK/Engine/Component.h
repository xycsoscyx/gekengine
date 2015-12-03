#pragma once

#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>
#include <typeindex>

#pragma warning(disable:4503)

namespace Gek
{
    DECLARE_INTERFACE_IID(ComponentType, "7B71BEF1-A2B2-4791-A664-2EA631E04392");

    DECLARE_INTERFACE_IID(Component, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E") : virtual public IUnknown
    {
        STDMETHOD_(LPCWSTR, getName)                (THIS) const PURE;
        STDMETHOD_(std::type_index, getIdentifier)  (THIS) const PURE;

        STDMETHOD_(LPVOID, create)                  (const std::unordered_map<CStringW, CStringW> &componentParameterList) PURE;
        STDMETHOD_(void, destroy)                   (LPVOID voidData) PURE;
    };
}; // namespace Gek
