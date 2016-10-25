/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
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

        GEK_INTERFACE(Core)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(InitializationFailed);

            Nano::Signal<void(void)> onResize;
            Nano::Signal<void(bool showCursor)> onInterface;

            virtual Xml::Node &getConfiguration(void) = 0;
            virtual Xml::Node const &getConfiguration(void) const = 0;

            virtual Plugin::Population * getPopulation(void) const = 0;
            virtual Plugin::Resources * getResources(void) const = 0;
            virtual Plugin::Renderer * getRenderer(void) const = 0;
        };
    }; // namespace Engine
}; // namespace Gek
