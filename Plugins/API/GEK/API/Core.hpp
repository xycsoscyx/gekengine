/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/JSON.hpp"
#include <wink/signal.hpp>
#include <imgui.h>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Population);
        GEK_PREDECLARE(Resources);
        GEK_PREDECLARE(Visualizer);
        GEK_PREDECLARE(Processor);

        GEK_INTERFACE(Core)
        {
            virtual ~Core(void) = default;

            wink::signal<wink::slot<void(void)>> onInitialized;
            wink::signal<wink::slot<void(bool &)>> canShutdown;
            wink::signal<wink::slot<void(void)>> onShutdown;

            template <typename TYPE>
            TYPE getOption(std::string_view system, std::string_view name, TYPE defaultValue)
            {
                return JSON::Value(getOption(system, name), defaultValue);
            }

            virtual JSON::Object getOption(std::string_view system, std::string_view name) const = 0;
            virtual void setOption(std::string_view system, std::string_view name, JSON::Object const &value) = 0;
            virtual void deleteOption(std::string_view system, std::string_view name) = 0;

            virtual Plugin::Population * getPopulation(void) const = 0;
            virtual Plugin::Resources * getResources(void) const = 0;
            virtual Plugin::Visualizer * getVisualizer(void) const = 0;

            virtual void listProcessors(std::function<void(Plugin::Processor *)> onProcessor) = 0;
        };
    }; // namespace Plugin
}; // namespace Gek
