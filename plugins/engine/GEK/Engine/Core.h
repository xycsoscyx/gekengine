#pragma once

#include "GEK\Context\Broadcaster.h"
#include "GEK\Utility\XML.h"
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

            virtual Xml::Root &getConfiguration(void) = 0;
            virtual Xml::Root const &getConfiguration(void) const = 0;

            virtual Plugin::Population * getPopulation(void) const = 0;
            virtual Plugin::Resources * getResources(void) const = 0;
            virtual Plugin::Renderer * getRenderer(void) const = 0;
        };
    }; // namespace Engine
}; // namespace Gek
