#pragma once

#include "GEK\Context\Observable.h"
#include <Windows.h>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Population);
        GEK_PREDECLARE(Resources);
        GEK_PREDECLARE(Renderer);

        GEK_INTERFACE(Core)
            : virtual public Observable
        {
            GEK_START_EXCEPTIONS();

            virtual Plugin::Population * getPopulation(void) const = 0;
            virtual Plugin::Resources * getResources(void) const = 0;
            virtual Plugin::Renderer * getRenderer(void) const = 0;

            virtual const String &getValue(const wchar_t *name, const wchar_t *attribute, const String &defaultValue = String()) const = 0;

            virtual void beginChanges(void) = 0;
            virtual void setValue(const wchar_t *name, const wchar_t *attribute, const wchar_t *value) = 0;
            virtual void finishChanges(bool commit) = 0;
        };

        struct ActionState
        {
            union
            {
                bool state;
                float value;
            };

            ActionState(bool state)
                : state(state)
            {
            }

            ActionState(float value)
                : value(value)
            {
            }
        };

        GEK_INTERFACE(CoreObserver)
            : public Observer
        {
            virtual void onChanged(void) { };

            virtual void onAction(const wchar_t *name, const ActionState &state) { };
        };
    }; // namespace Engine
}; // namespace Gek
