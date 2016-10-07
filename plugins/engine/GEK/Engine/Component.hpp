#pragma once

#include "GEK\Utility\XML.hpp"
#include "GEK\Context\Context.hpp"
#include <typeindex>
#include <imgui.h>

#pragma warning(disable:4503)

#define GEK_COMPONENT(TYPE)         struct TYPE : public Plugin::Component::Data

struct ImGuiContext;

namespace ImGui
{
    inline bool InputText(const char* label, Gek::String &string, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL)
    {
        Gek::StringUTF8 stringUTF8(string);
        stringUTF8.reserve(256);
        if (InputText(label, &stringUTF8.front(), 255, flags, callback, user_data))
        {
            string = stringUTF8;
            return true;
        }

        return false;
    }
}; // namespace ImGui

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Component)
        {
            struct Data
            {
                virtual ~Data(void) = default;
            };

            virtual const wchar_t * const getName(void) const = 0;
            virtual std::type_index getIdentifier(void) const = 0;

            virtual std::unique_ptr<Data> create(void) = 0;
            virtual void save(Data *data, Xml::Leaf &componentData) const = 0;
            virtual void load(Data *data, const Xml::Leaf &componentData) = 0;
        };
    }; // namespace Plugin

    namespace Edit
    {
        GEK_INTERFACE(Component)
            : public Plugin::Component
        {
            virtual void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data) = 0;
        };
    }; // namespace Edit
}; // namespace Gek
