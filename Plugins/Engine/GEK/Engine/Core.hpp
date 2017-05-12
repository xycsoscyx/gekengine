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
#include <nano_signal_slot.hpp>
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
            GEK_ADD_EXCEPTION(InvalidOptionName);

            GEK_INTERFACE(Log)
            {
                class Scope
                {
                private:
                    Log *log = nullptr;
                    std::string system, name;

                public:
                    Scope(Log *log, std::string const &system, std::string const &name)
                        : log(log)
                        , system(system)
                        , name(name)
                    {
                        log->beginEvent(system, name);
                    }

                    ~Scope(void)
                    {
                        log->endEvent(system, name);
                    }
                };

                enum class Type : uint8_t
                {
                    Message = 0,
                    Warning,
                    Error,
                    Debug,
                };

                virtual ~Log(void) = default;

                virtual void message(std::string const &system, Type logType, std::string const &message) = 0;

				template<typename TYPE, typename... PARAMETERS>
				void message(std::string const &system, Type logType, char const *formatting, TYPE const &value, PARAMETERS... arguments)
                {
					message(system, logType, String::Format(formatting, value, arguments...));
                }

                virtual void beginEvent(std::string const &system, std::string const &name) = 0;
                virtual void endEvent(std::string const &system, std::string const &name) = 0;

                virtual void setValue(std::string const &system, std::string const &name, float value) = 0;
                virtual void adjustValue(std::string const &system, std::string const &name, float value) = 0;
            };

            Nano::Signal<void(void)> onResize;
            Nano::Signal<void(ImGuiContext *context, ImGui::PanelManagerWindowData &windowData)> OnSettingsPanel;
            Nano::Signal<void(void)> onExit;

            virtual ~Core(void) = default;

            virtual bool update(void) = 0;

            virtual void setEditorState(bool enabled) = 0;
            virtual bool isEditorActive(void) const = 0;

            virtual Log * getLog(void) const = 0;
            virtual Window * getWindow(void) const = 0;
            virtual Video::Device * getVideoDevice(void) const = 0;

            virtual Plugin::Population * getPopulation(void) const = 0;
            virtual Plugin::Resources * getResources(void) const = 0;
            virtual Plugin::Renderer * getRenderer(void) const = 0;

            virtual ImGui::PanelManager * getPanelManager(void) = 0;

            virtual void listProcessors(std::function<void(Plugin::Processor *)> onProcessor) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
