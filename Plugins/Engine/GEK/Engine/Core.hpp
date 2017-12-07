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
#include "GEK/System/Window.hpp"
#include "GEK/System/VideoDevice.hpp"
#include <wink/signal.hpp>
#include <imgui.h>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Core)
            : public Plugin::Core
        {
            wink::signal<wink::slot<void(void)>> onChangedDisplay;
            wink::signal<wink::slot<void(void)>> onChangedSettings;

            virtual Window * getWindow(void) const = 0;

            virtual bool update(void) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
