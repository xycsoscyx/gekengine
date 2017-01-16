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
            GEK_ADD_EXCEPTION(InvalidIndexBufferFormat);
            GEK_ADD_EXCEPTION(InvalidOptionName);

            GEK_INTERFACE(Log)
            {
                struct Scope
                {
                    Log *log = nullptr;
                    const char *name = nullptr;
                    Scope(Log *log, const char *name)
                        : log(log)
                        , name(name)
                    {
                        log->beginEvent(name);
                    }

                    ~Scope(void)
                    {
                        log->endEvent(name);
                    }
                };

                enum class Type : uint8_t
                {
                    Message = 0,
                    Warning,
                    Error,
                    Debug,
                };

                virtual void message(const wchar_t *system, Type logType, const wchar_t *message) = 0;

                virtual void beginEvent(const char *name) = 0;
                virtual void endEvent(const char *name) = 0;

                virtual void addValue(const char *name, float value) = 0;
            };

            GEK_INTERFACE(Options)
            {
                struct Value
                {
                    enum class Type : uint8_t
                    {
                        Empty = 0,
                        Boolean,
                        Integer,
                        Float,
                        Vector,
                    };

                    Type type;
                    union
                    {
                        bool boolean;
                        int32_t integer;
                        float floater;
                        Math::Float4 vector;
                    };

                    Value(void);
                    Value(const Value &value);
                    void operator = (const Value &value);

                    Value(bool initialValue);
                    Value(int32_t initialValue);
                    Value(float initialValue);
                    Value(Math::Float4 initialValue);

                    operator bool();
                    operator int32_t();
                    operator float();
                    operator Math::Float4();
                };

                virtual void addValue(const wchar_t *name, const Value &value) = 0;
                virtual const Value &getValue(const wchar_t *name) const = 0;
            };

            Nano::Signal<void(void)> onResize;
            Nano::Signal<void(void)> onDisplay;
            Nano::Signal<void(bool showCursor)> onInterface;

            virtual ~Core(void) = default;

            virtual bool update(void) = 0;

            virtual void setEditorState(bool enabled) = 0;
            virtual bool isEditorActive(void) const = 0;

            virtual Log * getLog(void) const = 0;
            virtual Options * getOptions(void) const = 0;
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
