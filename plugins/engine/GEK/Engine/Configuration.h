#pragma once

#include "GEK\Context\Observer.h"

namespace Gek
{
    DECLARE_INTERFACE_IID(Configuration, "73A6CC8B-6E2B-4F97-8903-2D742E8BEDD8") : virtual public IUnknown
    {
        DECLARE_INTERFACE(Value) : virtual public IUnknown
        {
            STDMETHOD_(LPCWSTR, getName)                (THIS) PURE;
        };

        STDMETHOD_(Configuration &, getGroup)           (THIS_ LPCWSTR name) PURE;
        STDMETHOD_(Value &, getValue)                   (THIS_ LPCWSTR name) PURE;
    };

    DECLARE_INTERFACE_IID(ValueObserver, "7E5DB62D-D2BD-4E85-88D4-0BA81EC0DF46") : virtual public Observer
    {
        STDMETHOD_(void, onChanged)                     (THIS_ Configuration::Value *value) PURE;
    };
}; // namespace Gek
