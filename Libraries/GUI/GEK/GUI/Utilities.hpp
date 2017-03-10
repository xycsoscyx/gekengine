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
#include <imgui_internal.h>
#include <ImGuizmo.h>

namespace ImGui
{
    bool InputString(char const * label, ::Gek::String &string, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = nullptr, void *userData = nullptr);

    namespace Gek
    {
        bool InputFloat(char const * label, float *value, float step = 0.0f, float stepFast = 0.0f, int decimalPrecision = -1, ImGuiInputTextFlags flags = 0);
        bool InputFloat2(char const * label, float value[2], int decimalPrecision = -1, ImGuiInputTextFlags flags = 0);
        bool InputFloat3(char const * label, float value[3], int decimalPrecision = -1, ImGuiInputTextFlags flags = 0);
        bool InputFloat4(char const * label, float value[4], int decimalPrecision = -1, ImGuiInputTextFlags flags = 0);

        bool InputString(char const * label, ::Gek::String &string, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = nullptr, void *userData = nullptr);

        bool ListBox(char const * label, int *currentSelectionIndex, bool(*itemDataCallback)(void *userData, int index, const char ** textOutput), void *userData, int itemCount, int visibleItemCount = -1);

        void PlotLines(char const * label, float(*itemDataCallback)(void *userData, int index), void *userData, int itemCount, int itemStartIndex = 0, float scaleMinimum = std::numeric_limits<float>::max(), float scaleMaximum = std::numeric_limits<float>::max(), ImVec2 graphSize = ImVec2(0, 0));
        void PlotHistogram(char const * label, float(*itemDataCallback)(void *userData, int index), void *userData, int itemCount, int itemStartIndex = 0, float scaleMinimum = std::numeric_limits<float>::max(), float scaleMaximum = std::numeric_limits<float>::max(), ImVec2 graphSize = ImVec2(0, 0));
    }; // namespace Gek
}; // namespace ImGui
