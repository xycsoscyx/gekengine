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

        ImVec4 PushStyleColor(ImGuiCol idx, const ImVec4& col);
        float PushStyleVar(ImGuiStyleVar idx, float val);
        ImVec2 PushStyleVar(ImGuiStyleVar idx, const ImVec2& val);

        bool InputString(char const *label, std::string &string, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = nullptr, void* user_data = nullptr);

        bool CheckButton(char const *label, bool *storedState = nullptr, ImVec2 const &size = ImVec2(0.0f, 0.0f));
        bool CheckButton(char const *label, bool state, ImVec2 const &size = ImVec2(0.0f, 0.0f));

        bool RadioButton(char const *label, int *storedState, int buttonState, ImVec2 const &size = ImVec2(0.0f, 0.0f));

        void TextFrame(char const *label, ImVec2 const &requestedSize = ImVec2(0.0f, 0.0f), ImGuiButtonFlags flags = 0);

        bool SliderAngle2(char const *label, float v_rad[2], float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);
        bool SliderAngle3(char const *label, float v_rad[3], float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);
        bool SliderAngle4(char const *label, float v_rad[4], float v_degrees_min = -360.0f, float v_degrees_max = +360.0f);

        namespace Dock
        {
            enum class Position : uint8_t
            {
                Left = 0,
                Right,
                Top,
                Bottom,
                Tab,
                Float,
                None,
            };

            void Begin(char const *label = nullptr, ImVec2 const &workspace = ImVec2(0, 0), bool showBorder = false, ImVec2 const &splitSize = ImVec2(3.0f, 3.0f));
            void End(void);
            void Shutdown(void);
            void SetNextPosition(Position slot);
            bool BeginTab(char const *label, bool *opened = NULL, ImGuiWindowFlags extra_flags = 0, ImVec2 const &default_size = ImVec2(-1, -1));
            void EndTab(void);
            void SetActive(void);
        }; // Dock

        namespace Gizmo
        {
            enum class Operation
            {
                Translate = 0,
                Rotate,
                Scale,
                Bounds,
            };

            enum class Alignment
            {
                Local = 0,
                World,
            };

            void SetRect(float x, float y, float width, float height);
            void Manipulate(const float *view, const float *projection, Operation operation, Alignment alignment, float *matrix, float *deltaMatrix = 0, float *snap = 0, float *localBounds = NULL, float *boundsSnap = NULL);
        }; // namespace Gizmo
    }; // namespace UI
}; // namespace Gek
