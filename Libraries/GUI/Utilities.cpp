#include "GEK/GUI/Utilities.hpp"
#include "GEK/Math/Common.hpp"
#include "imgui_internal.h"
#include <unordered_map>

namespace Gek
{
    namespace UI
    {
        ImVec2 GetWindowContentRegionSize(void)
        {
            return (ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin());
        }

        ImVec4 PushStyleColor(ImGuiCol styleIndex, ImVec4 const &color)
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

        bool InputString(std::string_view label, std::string &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void *userData)
        {
            char text[256];
            strcpy(text, string.data());
            bool isChanged = ImGui::InputText(label.data(), text, 255, flags, callback, userData);
            if (isChanged)
            {
                string = text;
            }

            return isChanged;
        }

        bool CheckButton(std::string_view label, bool *storedState, ImVec2 const &size)
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

            bool isClicked = ImGui::Button(label.data(), size);
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

        bool CheckButton(std::string_view label, bool state, ImVec2 const &size)
        {
            return CheckButton(label, &state, size);
        }

        bool RadioButton(std::string_view label, int *storedState, int buttonState, ImVec2 const &size)
        {
            bool isClicked = CheckButton(label, *storedState == buttonState, size);
            if (isClicked)
            {
                *storedState = buttonState;
            }

            return isClicked;
        }

        void TextFrame(std::string_view label, ImVec2 const &requestedSize, ImGuiButtonFlags flags, ImU32 const *frameColor, ImColor const *textColor)
        {
            ImGuiWindow *window = ImGui::GetCurrentWindow();
            if (window->SkipItems)
            {
                return;
            }

            const ImGuiStyle &style = ImGui::GetStyle();
            const ImGuiID labelIdentity = window->GetID(label.data());
            const ImVec2 labelSize = ImGui::CalcTextSize(label.data(), nullptr, true);

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
                if (textColor)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, *textColor);
                }

                ImGui::RenderTextClipped(boundingBox.Min + style.FramePadding, boundingBox.Max - style.FramePadding, label.data(), &label.back() + 1, &labelSize, style.ButtonTextAlign, &boundingBox);
                if (textColor)
                {
                    ImGui::PopStyleColor();
                }
            }
        }

        bool SliderAngle2(std::string_view label, float radians[2], float degreesMinimum, float defgreedMaximum)
        {
            float degrees[]
            {
                radians[0] * 360.0f / (2 * IM_PI),
                radians[1] * 360.0f / (2 * IM_PI),
            };

            bool isChanged = ImGui::SliderFloat2(label.data(), degrees, degreesMinimum, defgreedMaximum, "%.0f deg", 1.0f);
            radians[0] = degrees[0] * (2 * IM_PI) / 360.0f;
            radians[1] = degrees[1] * (2 * IM_PI) / 360.0f;
            return isChanged;
        }

        bool SliderAngle3(std::string_view label, float radians[3], float degreesMinimum, float defgreedMaximum)
        {
            float degrees[]
            {
           radians[0] * 360.0f / (2 * IM_PI),
               radians[1] * 360.0f / (2 * IM_PI),
               radians[2] * 360.0f / (2 * IM_PI),
            };

            bool isChanged = ImGui::SliderFloat3(label.data(), degrees, degreesMinimum, defgreedMaximum, "%.0f deg", 1.0f);
            radians[0] = degrees[0] * (2 * IM_PI) / 360.0f;
            radians[1] = degrees[1] * (2 * IM_PI) / 360.0f;
            radians[2] = degrees[2] * (2 * IM_PI) / 360.0f;
            return isChanged;
        }

        bool SliderAngle4(std::string_view label, float radians[4], float degreesMinimum, float defgreedMaximum)
        {
            float degrees[]
            {
                radians[0] * 360.0f / (2 * IM_PI),
                radians[1] * 360.0f / (2 * IM_PI),
                radians[2] * 360.0f / (2 * IM_PI),
                radians[3] * 360.0f / (2 * IM_PI),
            };

            bool isChanged = ImGui::SliderFloat4(label.data(), degrees, degreesMinimum, defgreedMaximum, "%.0f deg", 1.0f);
            radians[0] = degrees[0] * (2 * IM_PI) / 360.0f;
            radians[1] = degrees[1] * (2 * IM_PI) / 360.0f;
            radians[2] = degrees[2] * (2 * IM_PI) / 360.0f;
            radians[3] = degrees[3] * (2 * IM_PI) / 360.0f;
            return isChanged;
        }

		bool ToggleButton(const char* label, bool* state)
		{
			ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			float height = ImGui::GetItemRectSize().y;
			float width = height * 1.55f;
			float radius = height * 0.50f;

			ImGui::InvisibleButton(label, ImVec2(width, height));
			bool changed = ImGui::IsItemClicked();
			if (changed)
			{
				*state = !*state;
			}

			float t = *state ? 1.0f : 0.0f;

			ImGuiContext& context = *GImGui;
			float ANIM_SPEED = 0.08f;
			if (context.LastActiveId == context.CurrentWindow->GetID(label))// && context.LastActiveIdTimer < ANIM_SPEED)
			{
				float t_anim = ImSaturate(context.LastActiveIdTimer / ANIM_SPEED);
				t = *state ? (t_anim) : (1.0f - t_anim);
			}

			ImU32 col_bg;
			if (ImGui::IsItemHovered())
			{
				col_bg = ImGui::GetColorU32(Math::Blend(ImVec4(0.78f, 0.78f, 0.78f, 1.0f), ImVec4(0.64f, 0.83f, 0.34f, 1.0f), t));
			}
			else
			{
				col_bg = ImGui::GetColorU32(Math::Blend(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), ImVec4(0.56f, 0.83f, 0.26f, 1.0f), t));
			}

			drawList->AddRectFilled(cursorPosition, ImVec2(cursorPosition.x + width, cursorPosition.y + height), col_bg, height * 0.5f);
			drawList->AddCircleFilled(ImVec2(cursorPosition.x + radius + t * (width - radius * 2.0f), cursorPosition.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
			return changed;
		}
	}; // namespace UI
}; // namespace Gek