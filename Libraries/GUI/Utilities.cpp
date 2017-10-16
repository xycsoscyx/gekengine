#include "GEK/GUI/Utilities.hpp"
#include <unordered_map>

namespace Gek
{
    namespace UI
    {
        ImVec2 GetWindowContentRegionSize(void)
        {
            return (ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin());
        }

        ImVec4 PushStyleColor(ImGuiCol styleIndex, const ImVec4& color)
        {
            ImVec4 oldValue = ImGui::GetStyle().Colors[styleIndex];
            ImGui::PushStyleColor(styleIndex, color);
            return oldValue;
        }

#define GET_STYLE(OFFSET) (void *)((unsigned char *)&GImGui->Style + OFFSET)

        static const void *GlobalStyleModifiers[ImGuiStyleVar_Count_] =
        {
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, Alpha)),                // ImGuiStyleVar_Alpha
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, WindowPadding)),        // ImGuiStyleVar_WindowPadding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, WindowRounding)),       // ImGuiStyleVar_WindowRounding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, WindowMinSize)),        // ImGuiStyleVar_WindowMinSize
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, ChildWindowRounding)),  // ImGuiStyleVar_ChildWindowRounding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, FramePadding)),         // ImGuiStyleVar_FramePadding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, FrameRounding)),        // ImGuiStyleVar_FrameRounding
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, ItemSpacing)),          // ImGuiStyleVar_ItemSpacing
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, ItemInnerSpacing)),     // ImGuiStyleVar_ItemInnerSpacing
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, IndentSpacing)),        // ImGuiStyleVar_IndentSpacing
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, GrabMinSize)),          // ImGuiStyleVar_GrabMinSize
            GET_STYLE((ImU32)IM_OFFSETOF(ImGuiStyle, ButtonTextAlign)),      // ImGuiStyleVar_ButtonTextAlign
        };

        float PushStyleVar(ImGuiStyleVar styleIndex, float value)
        {
            float oldValue = *(float *)GlobalStyleModifiers[styleIndex];
            ImGui::PushStyleVar(styleIndex, value);
            return oldValue;
        }

        ImVec2 PushStyleVar(ImGuiStyleVar styleIndex, ImVec2 const &value)
        {
            ImVec2 oldValue = *(ImVec2 *)GlobalStyleModifiers[styleIndex];
            ImGui::PushStyleVar(styleIndex, value);
            return oldValue;
        }

        bool InputString(char const *label, std::string &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void *userData)
        {
            char text[256];
            strcpy(text, string.c_str());
            bool isChanged = ImGui::InputText(label, text, 255, flags, callback, userData);
            if (isChanged)
            {
                string = text;
            }

            return isChanged;
        }

        bool CheckButton(char const *label, bool *storedState, ImVec2 const &size)
        {
            const bool state = storedState ? *storedState : false;
            if (!state)
            {
                auto const &style(ImGui::GetStyle());
                ImVec4 CheckButtonColor = ImVec4(1.0f, 1.0f, 1.0f, 0.0f) - style.Colors[ImGuiCol_Button];
                ImVec4 CheckButtonActiveColor = ImVec4(1.0f, 1.0f, 1.0f, 0.0f) - style.Colors[ImGuiCol_ButtonActive];
                ImGui::PushStyleColor(ImGuiCol_Button, CheckButtonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, CheckButtonActiveColor);
            }

            bool isClicked = ImGui::Button(label, size);
            if (isClicked && storedState)
            {
                *storedState = !(*storedState);
            }

            if (!state)
            {
                ImGui::PopStyleColor(2);
            }

            return isClicked;
        }

        bool CheckButton(char const *label, bool state, ImVec2 const &size)
        {
            return CheckButton(label, &state, size);
        }

        bool RadioButton(char const *label, int *storedState, int buttonState, ImVec2 const &size)
        {
            bool isClicked = CheckButton(label, *storedState == buttonState, size);
            if (isClicked)
            {
                *storedState = buttonState;
            }

            return isClicked;
        }

        void TextFrame(char const *label, ImVec2 const &requestedSize, ImGuiButtonFlags flags, ImU32 *frameColor)
        {
            ImGuiWindow *window = ImGui::GetCurrentWindow();
            if (window->SkipItems)
            {
                return;
            }

            const ImGuiStyle &style = ImGui::GetStyle();
            const ImGuiID labelIdentity = window->GetID(label);
            const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);

            ImVec2 position = window->DC.CursorPos;
            // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
            if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset)
            {
                position.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
            }

            ImVec2 size = ImGui::CalcItemSize(requestedSize, labelSize.x + style.FramePadding.x * 2.0f, labelSize.y + style.FramePadding.y * 2.0f);

            const ImRect boundingBox(position, position + size);
            ImGui::ItemSize(boundingBox, style.FramePadding.y);
            if (ImGui::ItemAdd(boundingBox, &labelIdentity))
            {
                const ImU32 color = (frameColor ? *frameColor : ImGui::GetColorU32(ImGuiCol_Button));
                ImGui::RenderFrame(boundingBox.Min, boundingBox.Max, color, true, style.FrameRounding);
                ImGui::RenderTextClipped(boundingBox.Min + style.FramePadding, boundingBox.Max - style.FramePadding, label, nullptr, &labelSize, style.ButtonTextAlign, &boundingBox);
            }
        }

        bool SliderAngle2(char const *label, float radians[2], float degreesMinimum, float defgreedMaximum)
        {
            float degrees[]
            {
                radians[0] * 360.0f / (2 * IM_PI),
                radians[1] * 360.0f / (2 * IM_PI),
            };

            bool isChanged = ImGui::SliderFloat2(label, degrees, degreesMinimum, defgreedMaximum, "%.0f deg", 1.0f);
            radians[0] = degrees[0] * (2 * IM_PI) / 360.0f;
            radians[1] = degrees[1] * (2 * IM_PI) / 360.0f;
            return isChanged;
        }

        bool SliderAngle3(char const *label, float radians[3], float degreesMinimum, float defgreedMaximum)
        {
            float degrees[]
            {
           radians[0] * 360.0f / (2 * IM_PI),
               radians[1] * 360.0f / (2 * IM_PI),
               radians[2] * 360.0f / (2 * IM_PI),
            };

            bool isChanged = ImGui::SliderFloat3(label, degrees, degreesMinimum, defgreedMaximum, "%.0f deg", 1.0f);
            radians[0] = degrees[0] * (2 * IM_PI) / 360.0f;
            radians[1] = degrees[1] * (2 * IM_PI) / 360.0f;
            radians[2] = degrees[2] * (2 * IM_PI) / 360.0f;
            return isChanged;
        }

        bool SliderAngle4(char const *label, float radians[4], float degreesMinimum, float defgreedMaximum)
        {
            float degrees[]
            {
           radians[0] * 360.0f / (2 * IM_PI),
               radians[1] * 360.0f / (2 * IM_PI),
               radians[2] * 360.0f / (2 * IM_PI),
               radians[3] * 360.0f / (2 * IM_PI),
            };

            bool isChanged = ImGui::SliderFloat4(label, degrees, degreesMinimum, defgreedMaximum, "%.0f deg", 1.0f);
            radians[0] = degrees[0] * (2 * IM_PI) / 360.0f;
            radians[1] = degrees[1] * (2 * IM_PI) / 360.0f;
            radians[2] = degrees[2] * (2 * IM_PI) / 360.0f;
            radians[3] = degrees[3] * (2 * IM_PI) / 360.0f;
            return isChanged;
        }
    }; // namespace UI
}; // namespace Gek