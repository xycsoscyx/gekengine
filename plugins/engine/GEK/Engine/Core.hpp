#pragma once

#include "GEK\Utility\Context.hpp"
#include "GEK\Utility\XML.hpp"
#include <nano_signal_slot.hpp>
#include <Windows.h>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Population);
        GEK_PREDECLARE(Resources);
        GEK_PREDECLARE(Renderer);

        GEK_INTERFACE(Configuration)
        {
            virtual operator Xml::Node &() = 0;
        };

        struct ActionParameter
        {
            union
            {
                bool state;
                float value;
            };

            ActionParameter(void)
            {
            }

            ActionParameter(bool state)
                : state(state)
            {
            }

            ActionParameter(float value)
                : value(value)
            {
            }
        };

        GEK_INTERFACE(Core)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(InitializationFailed);

            Nano::Signal<void(void)> onResize;
            Nano::Signal<void(void)> onConfigurationChanged;
            Nano::Signal<void(const wchar_t *actionName, const ActionParameter &actionParameter)> onAction;

            virtual ConfigurationPtr changeConfiguration(void) = 0;
            virtual Xml::Node const &getConfiguration(void) const = 0;

            virtual Plugin::Population * getPopulation(void) const = 0;
            virtual Plugin::Resources * getResources(void) const = 0;
            virtual Plugin::Renderer * getRenderer(void) const = 0;
        };
    }; // namespace Engine
}; // namespace Gek
