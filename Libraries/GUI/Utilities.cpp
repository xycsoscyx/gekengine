#include "GEK/GUI/Utilities.hpp"

namespace Gek
{
    namespace UI
    {
        bool InputString(char const *label, std::string &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void *userData)
        {
            char text[256];
            strcpy(text, string.c_str());
            bool changed = ImGui::InputText(label, text, 255, flags, callback, userData);
            if (changed)
            {
                string = text;
            }

            return changed;
        }

        bool CheckButton(char const *label, bool *storedState, ImVec2 const &size)
        {
            const bool state = storedState ? *storedState : false;
            if (!state)
            {
                const auto &style(ImGui::GetStyle());
                ImVec4 CheckButtonColor = ImVec4(1.0f, 1.0f, 1.0f, 0.0f) - style.Colors[ImGuiCol_Button]; 
                ImVec4 CheckButtonActiveColor = ImVec4(1.0f, 1.0f, 1.0f, 0.0f) - style.Colors[ImGuiCol_ButtonActive];
                ImGui::PushStyleColor(ImGuiCol_Button, CheckButtonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, CheckButtonActiveColor);
            }

            bool clicked = ImGui::Button(label, size);
            if (clicked && storedState)
            {
                *storedState = !(*storedState);
            }

            if (!state)
            {
                ImGui::PopStyleColor(2);
            }

            return clicked;
        }

        bool CheckButton(char const *label, bool state, ImVec2 const &size)
        {
            return CheckButton(label, &state, size);
        }

        bool RadioButton(char const *label, int *storedState, int buttonState, ImVec2 const &size)
        {
            bool clicked = CheckButton(label, *storedState == buttonState, size);
            if (clicked)
            {
                *storedState = buttonState;
            }

            return clicked;
        }

        void TextFrame(char const *label, ImVec2 const &requestedSize, ImGuiButtonFlags flags)
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
                const ImU32 color = ImGui::GetColorU32(ImGuiCol_Button);
                ImGui::RenderFrame(boundingBox.Min, boundingBox.Max, color, true, style.FrameRounding);
                ImGui::RenderTextClipped(boundingBox.Min + style.FramePadding, boundingBox.Max - style.FramePadding, label, nullptr, &labelSize, style.ButtonTextAlign, &boundingBox);
            }
        }
    }; // namespace UI
}; // namespace Gek