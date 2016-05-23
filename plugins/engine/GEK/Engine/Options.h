#pragma once

#include "GEK\Context\Observer.h"
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
    interface Options
    {
        const CStringW &getValue(LPCWSTR name, LPCWSTR attribute, const CStringW &defaultValue = L"") const;

        void beginChanges(void);
        void setValue(LPCWSTR name, LPCWSTR attribute, LPCWSTR value);
        void finishChanges(bool commit);
    };

    interface OptionsObserver
    {
        void onChanged(void)
    };
}; // namespace Gek
