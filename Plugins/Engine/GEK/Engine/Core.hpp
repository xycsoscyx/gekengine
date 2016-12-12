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
#include <nano_signal_slot.hpp>
#include <Windows.h>
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
            GEK_ADD_EXCEPTION(InitializationFailed);
            GEK_ADD_EXCEPTION(InvalidDisplayMode);
            GEK_ADD_EXCEPTION(InvalidIndexBufferFormat);

            enum class LogType : uint8_t
            {
                Message = 0,
                Warning,
                Error,
                Debug,
            };

            Nano::Signal<void(void)> onResize;
            Nano::Signal<void(void)> onDisplay;
            Nano::Signal<void(bool showCursor)> onInterface;

            virtual ~Core(void) = default;

            virtual void log(const wchar_t *system, LogType logType, const wchar_t *message) = 0;

            virtual JSON::Object &getConfiguration(void) = 0;
            virtual JSON::Object const &getConfiguration(void) const = 0;
            virtual bool isEditorActive(void) const = 0;

            virtual Plugin::Population * getPopulation(void) const = 0;
            virtual Plugin::Resources * getResources(void) const = 0;
            virtual Plugin::Renderer * getRenderer(void) const = 0;

            virtual ImGui::PanelManager * getPanelManager(void) = 0;

            virtual void listProcessors(std::function<void(Plugin::Processor *)> onProcessor) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
