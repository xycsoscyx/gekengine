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
#include "GEK/System/Window.hpp"
#include "GEK/System/VideoDevice.hpp"
#include <lsignal.h>
#include <imgui.h>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Population);
        GEK_PREDECLARE(Resources);
        GEK_PREDECLARE(Renderer);
        GEK_PREDECLARE(Processor);

        GEK_INTERFACE(Core)
        {
            using ListenerHandle = uint64_t;

            lsignal::signal<void(void)> onResize;
            lsignal::signal<void(void)> onSettingsChanged;
            lsignal::signal<void(void)> onExit;

            virtual ~Core(void) = default;

            virtual JSON::Reference getOption(std::string const &system, std::string const &name) = 0;
            virtual void setOption(std::string const &system, std::string const &name, JSON::Object const &value) = 0;
            virtual void deleteOption(std::string const &system, std::string const &name) = 0;

            virtual Window * getWindow(void) const = 0;
            virtual Video::Device * getVideoDevice(void) const = 0;

            virtual Plugin::Population * getPopulation(void) const = 0;
            virtual Plugin::Resources * getResources(void) const = 0;
            virtual Plugin::Renderer * getRenderer(void) const = 0;

            virtual void listProcessors(std::function<void(Plugin::Processor *)> onProcessor) = 0;

            virtual bool update(void) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
