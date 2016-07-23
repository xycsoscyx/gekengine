#pragma once

#include "GEK\Context\Broadcaster.h"
#include <Windows.h>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Population);
        GEK_PREDECLARE(Resources);
        GEK_PREDECLARE(Renderer);

        struct ActionParameter
        {
            union
            {
                bool state;
                float value;
            };

            ActionParameter(bool state)
                : state(state)
            {
            }

            ActionParameter(float value)
                : value(value)
            {
            }
        };

        GEK_INTERFACE(CoreListener)
        {
            virtual void onChanged(void) { };

            virtual void onAction(const wchar_t *actionName, const ActionParameter &parameter) { };
        };

        GEK_INTERFACE(Core)
            : public Broadcaster<CoreListener>
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
    }; // namespace Engine
}; // namespace Gek
