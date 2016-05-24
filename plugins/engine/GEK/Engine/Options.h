#pragma once

#include "GEK\Context\Observer.h"
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
    GEK_INTERFACE(Options)
    {
        virtual const CStringW &getValue(LPCWSTR name, LPCWSTR attribute, const CStringW &defaultValue = L"") const = 0;

        virtual void beginChanges(void) = 0;
        virtual void setValue(LPCWSTR name, LPCWSTR attribute, LPCWSTR value) = 0;
        virtual void finishChanges(bool commit) = 0;
    };

    GEK_INTERFACE(OptionsObserver)
    {
        virtual void onChanged(void) = 0;
    };
}; // namespace Gek
