/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/String.hpp"

#include <imgui.h>
#include <ImGuizmo.h>

namespace ImGui
{
    bool InputString(char const * const label, ::Gek::String &string, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);

    namespace Gek
    {
        bool InputFloat(char const * const label, float* v, float step = 0.0f, float step_fast = 0.0f, int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
        bool InputFloat2(char const * const label, float v[2], int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
        bool InputFloat3(char const * const label, float v[3], int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);
        bool InputFloat4(char const * const label, float v[4], int decimal_precision = -1, ImGuiInputTextFlags extra_flags = 0);

        bool InputString(char const * const label, ::Gek::String &string, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);

        bool ListBox(char const * const label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items = -1);
    }; // namespace Gek
}; // namespace ImGui
