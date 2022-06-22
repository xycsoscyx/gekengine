#include "GEK/GUI/Utilities.hpp"
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector4.hpp"
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

        struct ImGuiStyleVarInfo
        {
            ImGuiDataType   Type;
            ImU32           Count;
            ImU32           Offset;
            void*           GetVarPtr(ImGuiStyle* style) const { return (void*)((unsigned char*)style + Offset); }
        };

        static const ImGuiStyleVarInfo GStyleVarInfo[] =
        {
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, Alpha) },              // ImGuiStyleVar_Alpha
            { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowPadding) },      // ImGuiStyleVar_WindowPadding
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowRounding) },     // ImGuiStyleVar_WindowRounding
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowBorderSize) },   // ImGuiStyleVar_WindowBorderSize
            { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowMinSize) },      // ImGuiStyleVar_WindowMinSize
            { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, WindowTitleAlign) },   // ImGuiStyleVar_WindowTitleAlign
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildRounding) },      // ImGuiStyleVar_ChildRounding
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ChildBorderSize) },    // ImGuiStyleVar_ChildBorderSize
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupRounding) },      // ImGuiStyleVar_PopupRounding
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, PopupBorderSize) },    // ImGuiStyleVar_PopupBorderSize
            { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, FramePadding) },       // ImGuiStyleVar_FramePadding
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameRounding) },      // ImGuiStyleVar_FrameRounding
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, FrameBorderSize) },    // ImGuiStyleVar_FrameBorderSize
            { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemSpacing) },        // ImGuiStyleVar_ItemSpacing
            { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ItemInnerSpacing) },   // ImGuiStyleVar_ItemInnerSpacing
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, IndentSpacing) },      // ImGuiStyleVar_IndentSpacing
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarSize) },      // ImGuiStyleVar_ScrollbarSize
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, ScrollbarRounding) },  // ImGuiStyleVar_ScrollbarRounding
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabMinSize) },        // ImGuiStyleVar_GrabMinSize
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, GrabRounding) },       // ImGuiStyleVar_GrabRounding
            { ImGuiDataType_Float, 1, (ImU32)IM_OFFSETOF(ImGuiStyle, TabRounding) },        // ImGuiStyleVar_TabRounding
            { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, ButtonTextAlign) },    // ImGuiStyleVar_ButtonTextAlign
            { ImGuiDataType_Float, 2, (ImU32)IM_OFFSETOF(ImGuiStyle, SelectableTextAlign) },// ImGuiStyleVar_SelectableTextAlign
        };

        static const ImGuiStyleVarInfo* GetStyleVarInfo(ImGuiStyleVar idx)
        {
            IM_ASSERT(idx >= 0 && idx < ImGuiStyleVar_COUNT);
            IM_ASSERT(IM_ARRAYSIZE(GStyleVarInfo) == ImGuiStyleVar_COUNT);
            return &GStyleVarInfo[idx];
        }

        float PushStyleVar(ImGuiStyleVar index, float newValue)
        {
            const ImGuiStyleVarInfo* variableInformation = GetStyleVarInfo(index);
            if (variableInformation->Type == ImGuiDataType_Float && variableInformation->Count == 1)
            {
                ImGuiContext& imGuiContext = *GImGui;
                float* activeValue = (float*)variableInformation->GetVarPtr(&imGuiContext.Style);
                auto oldValue = *activeValue;
                imGuiContext.StyleVarStack.push_back(ImGuiStyleMod(index, *activeValue));
                *activeValue = newValue;
                return oldValue;
            }

            IM_ASSERT(0); // Called function with wrong-type? Variable is not a float.
            return float();
        }

        ImVec2 PushStyleVar(ImGuiStyleVar index, const ImVec2& newValue)
        {
            const ImGuiStyleVarInfo* variableInformation = GetStyleVarInfo(index);
            if (variableInformation->Type == ImGuiDataType_Float && variableInformation->Count == 2)
            {
                ImGuiContext& imGuiContext = *GImGui;
                ImVec2* activeValue = (ImVec2*)variableInformation->GetVarPtr(&imGuiContext.Style);
                auto oldValue = *activeValue;
                imGuiContext.StyleVarStack.push_back(ImGuiStyleMod(index, *activeValue));
                *activeValue = newValue;
                return oldValue;
            }

            IM_ASSERT(0); // Called function with wrong-type? Variable is not a ImVec2.
            return ImVec2();
        }

        bool InputString(std::string_view label, std::string &string, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *userData)
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

        void TextFrame(std::string_view label, ImVec2 const &requestedSize, ImU32 const *frameColor, ImVec4 const *textColor)
        {
            ImGuiButtonFlags flags = 0;
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
            if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset)
            {
                position.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
            }

            ImVec2 size = ImGui::CalcItemSize(requestedSize, labelSize.x + style.FramePadding.x * 2.0f, labelSize.y + style.FramePadding.y * 2.0f);

            const ImRect boundingBox(position, position + size);
            ImGui::ItemSize(boundingBox, style.FramePadding.y);
            if (ImGui::ItemAdd(boundingBox, labelIdentity))
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

        bool ToggleButton(char const *label, bool *state)
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
            /*if (context.LastActiveId == context.CurrentWindow->GetID(label)) && context.LastActiveIdTimer < ANIM_SPEED)
            {
                float t_anim = ImSaturate(context.LastActiveIdTimer / ANIM_SPEED);
                t = *state ? (t_anim) : (1.0f - t_anim);
            }*/

            ImU32 col_bg;
            if (ImGui::IsItemHovered())
            {
                static const auto UnClicked = Math::Float4(0.78f, 0.78f, 0.78f, 1.0f);
                static const auto Clicked = Math::Float4(0.64f, 0.83f, 0.34f, 1.0f);
                col_bg = ImGui::GetColorU32(*(ImVec4 *)&Math::Blend(UnClicked, Clicked, t));
            }
            else
            {
                static const auto UnClicked = Math::Float4(0.85f, 0.85f, 0.85f, 1.0f);
                static const auto Clicked = Math::Float4(0.56f, 0.83f, 0.26f, 1.0f);
                col_bg = ImGui::GetColorU32(*(ImVec4 *)&Math::Blend(UnClicked, Clicked, t));
            }

            drawList->AddRectFilled(cursorPosition, ImVec2(cursorPosition.x + width, cursorPosition.y + height), col_bg, height * 0.5f);
            drawList->AddCircleFilled(ImVec2(cursorPosition.x + radius + t * (width - radius * 2.0f), cursorPosition.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
            return changed;
        }

        bool Input(std::string_view label, bool *value)
        {
            return ImGui::Checkbox(label.data(), value);
        }

        bool Input(std::string_view label, int32_t *value)
        {
            return ImGui::InputInt(label.data(), value);
        }

        bool Input(std::string_view label, uint32_t *value)
        {
            int32_t simplifiedValue = *value;
            bool changed = ImGui::InputInt(label.data(), &simplifiedValue);
            if (changed)
            {
                *value = simplifiedValue;
            }

            return changed;
        }

        bool Input(std::string_view label, int64_t *value)
        {
            int32_t simplifiedValue = *value;
            bool changed = ImGui::InputInt(label.data(), &simplifiedValue);
            if (changed)
            {
                *value = simplifiedValue;
            }

            return changed;
        }

        bool Input(std::string_view label, uint64_t *value)
        {
            int32_t simplifiedValue = *value;
            bool changed = ImGui::InputInt(label.data(), &simplifiedValue);
            if (changed)
            {
                *value = simplifiedValue;
            }

            return changed;
        }

        bool Input(std::string_view label, float *value)
        {
            return ImGui::InputFloat(label.data(), value);
        }

        bool Input(std::string_view label, std::string *value)
        {
            return InputString(label, *value);
        }
    }; // namespace UI
}; // namespace Gek