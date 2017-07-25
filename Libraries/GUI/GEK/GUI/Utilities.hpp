/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Utility/String.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

#include "GEK/GUI/IconsFontAwesome.h"
#include "GEK/GUI/IconsMaterialDesign.h"
namespace Gek
{
    namespace UI
    {
        bool InputString(const char* label, std::string &string, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = nullptr, void* user_data = nullptr);

        bool CheckButton(char const *label, bool *storedState = nullptr, ImVec2 const &size = ImVec2(0.0f, 0.0f));
        bool CheckButton(char const *label, bool state, ImVec2 const &size = ImVec2(0.0f, 0.0f));

        bool RadioButton(char const *label, int *storedState, int buttonState, ImVec2 const &size = ImVec2(0.0f, 0.0f));

        void TextFrame(char const *label, ImVec2 const &requestedSize = ImVec2(0.0f, 0.0f), ImGuiButtonFlags flags = 0);
    }; // namespace UI
}; // namespace Gek