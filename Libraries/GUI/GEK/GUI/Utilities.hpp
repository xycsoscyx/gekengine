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
        ImVec2 GetWindowContentRegionSize();

        bool InputString(std::string_view label, const std::string &string, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
        bool InputString(std::string_view label, std::string &string, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);

        bool CheckButton(std::string_view label, bool *storedState = nullptr, ImVec2 const &size = ImVec2(0.0f, 0.0f));
        bool CheckButton(std::string_view label, bool state, ImVec2 const &size = ImVec2(0.0f, 0.0f));

        template <typename TYPE>
        bool RadioButton(std::string_view label, TYPE *storedState, TYPE buttonState, ImVec2 const &size)
        {
            bool isClicked = CheckButton(label, *storedState == buttonState, size);
            if (isClicked)
            {
                *storedState = buttonState;
            }

            return isClicked;
        }

        void TextFrame(std::string_view label, ImVec2 const &requestedSize = ImVec2(0.0f, 0.0f), ImU32 const *frameColor = nullptr, ImVec4 const *textColor = nullptr);

        bool SliderAngle2(std::string_view label, float v_rad[2], float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);
        bool SliderAngle3(std::string_view label, float v_rad[3], float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);
        bool SliderAngle4(std::string_view label, float v_rad[4], float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);

        bool Input(std::string_view label, bool *value);
        bool Input(std::string_view label, int32_t *value);
        bool Input(std::string_view label, uint32_t *value);
        bool Input(std::string_view label, int64_t *value);
        bool Input(std::string_view label, uint64_t *value);
        bool Input(std::string_view label, float *value);
        bool Input(std::string_view label, std::string *value);
    }; // namespace UI
}; // namespace Gek
