#pragma once

#include "GEK\Utility\String.h"
#include "GEK\Context\Observable.h"

namespace Gek
{
    GEK_INTERFACE(Options)
        : virtual public Observable
    {
        virtual const wstring &getValue(const wchar_t *name, const wchar_t *attribute, const wstring &defaultValue = L"") const = 0;

        virtual void beginChanges(void) = 0;
        virtual void setValue(const wchar_t *name, const wchar_t *attribute, const wchar_t *value) = 0;
        virtual void finishChanges(bool commit) = 0;
    };

    GEK_INTERFACE(OptionsObserver)
        : virtual public Observer
    {
        virtual void onChanged(void) { };
    };
}; // namespace Gek
