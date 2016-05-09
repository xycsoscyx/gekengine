#pragma once

#include "GEK\Context\Observer.h"
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
    DECLARE_INTERFACE_IID(Options, "73A6CC8B-6E2B-4F97-8903-2D742E8BEDD8") : virtual public IUnknown
    {
        STDMETHOD_(const CStringW &, getValue)          (THIS_ LPCWSTR name, LPCWSTR attribute, const CStringW &defaultValue = L"") CONST PURE;

        STDMETHOD_(void, beginChanges)                  (THIS) PURE;
        STDMETHOD_(void, setValue)                      (THIS_ LPCWSTR name, LPCWSTR attribute, LPCWSTR value) PURE;
        STDMETHOD_(void, finishChanges)                 (THIS_ bool commit) PURE;
    };

    DECLARE_INTERFACE_IID(OptionsObserver, "7E5DB62D-D2BD-4E85-88D4-0BA81EC0DF46") : virtual public Observer
    {
        STDMETHOD_(void, onChanged)                     (THIS) PURE;
    };
}; // namespace Gek
