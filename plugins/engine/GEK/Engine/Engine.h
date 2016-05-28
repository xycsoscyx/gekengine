#pragma once

#include "GEK\Context\Observable.h"

namespace Gek
{
    GEK_PREDECLARE(Population);
    GEK_PREDECLARE(Resources);
    GEK_PREDECLARE(Render);

    GEK_INTERFACE(EngineContext)
        : virtual public Observable
    {
        virtual Population * getPopulation(void) const = 0;
        virtual Resources * getResources(void) const = 0;
        virtual Render * getRender(void) const = 0;
    };

    GEK_INTERFACE(Engine)
        : public EngineContext
    {
        virtual LRESULT windowEvent(UINT32 message, WPARAM wParam, LPARAM lParam) = 0;

        virtual bool update(void) = 0;
    };

    struct ActionParam
    {
        union
        {
            bool state;
            float value;
        };

        ActionParam(bool state)
            : state(state)
        {
        }

        ActionParam(float value)
            : value(value)
        {
        }
    };

    GEK_INTERFACE(EngineObserver)
        : virtual public Observer
    {
        virtual void onAction(const wchar_t *name, const ActionParam &param) { };
    };
}; // namespace Gek
