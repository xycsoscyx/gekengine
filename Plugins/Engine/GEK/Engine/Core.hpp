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
#include "GEK/API/Core.hpp"
#include "GEK/System/WindowDevice.hpp"
#include "GEK/System/RenderDevice.hpp"
#include <wink/signal.hpp>
#include <imgui.h>

namespace Gek
{
    namespace Engine
    {
        GEK_PREDECLARE(Population);
        GEK_PREDECLARE(Resources);

        GEK_INTERFACE(Core)
            : public Plugin::Core
        {
			wink::signal<wink::slot<void(void)>> onChangedDisplay;
            wink::signal<wink::slot<void(void)>> onChangedSettings;

            virtual ~Core(void) = default;

            virtual Window::Device * getWindow(void) const = 0;
            virtual Render::Device * getRenderDevice(void) const = 0;

            virtual Engine::Population * getFullPopulation(void) const = 0;
            virtual Engine::Resources * getFullResources(void) const = 0;
        };
    }; // namespace Engine
}; // namespace Gek
