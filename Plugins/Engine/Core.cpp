#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Timer.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/System/AudioDevice.hpp"
#include "GEK/Engine/Application.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Renderer.hpp"
#include <concurrent_queue.h>
#include <imgui_internal.h>
#include <queue>
#include <ppl.h>

namespace ImGui
{
    enum ImGuiStyleEnum
    {
        ImGuiStyle_Default = 0,
        ImGuiStyle_Gray,        // This is the default theme of my main.cpp demo.
        ImGuiStyle_Light,
        ImGuiStyle_OSX,         // Posted by @itamago here: https://github.com/ocornut/imgui/pull/511 (hope I can use it)
        ImGuiStyle_OSXOpaque,   // Posted by @dougbinks here: https://gist.github.com/dougbinks/8089b4bbaccaaf6fa204236978d165a9 (hope I can use it)
        ImGuiStyle_DarkOpaque,
        ImGuiStyle_Soft,        // Posted by @olekristensen here: https://github.com/ocornut/imgui/issues/539 (hope I can use it)
        ImGuiStyle_OSXInverse,
        ImGuiStyle_OSXOpaqueInverse,
        ImGuiStyle_DarkOpaqueInverse,
    };

    void ChangeStyleColors(ImGuiStyle& style, float satThresholdForInvertingLuminance, float shiftHue)
    {
        if (satThresholdForInvertingLuminance >= 1.0f && shiftHue == 0.0f)
        {
            return;
        }

        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            ImVec4& col = style.Colors[i];

            float H, S, V;
            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);
            if (S <= satThresholdForInvertingLuminance)
            {
                V = 1.0f - V;
            }

            if (shiftHue)
            {
                H += shiftHue;
                if (H>1) H -= 1.0f;
                else if (H<0) H += 1.0f;
            }

            ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
        }
    }

    static inline void InvertStyleColors(ImGuiStyle& style)
    {
        ChangeStyleColors(style, 0.1f, 0.0f);
    }

    static inline void ChangeStyleColorsHue(ImGuiStyle& style, float shiftHue = 0.0f)
    {
        ChangeStyleColors(style, 0.0f, shiftHue);
    }

    static inline ImVec4 ConvertTitleBgColFromPrevVersion(const ImVec4& win_bg_col, const ImVec4& title_bg_col)
    {
        float new_a = 1.0f - ((1.0f - win_bg_col.w) * (1.0f - title_bg_col.w)), k = title_bg_col.w / new_a;
        return ImVec4((win_bg_col.x * win_bg_col.w + title_bg_col.x) * k, (win_bg_col.y * win_bg_col.w + title_bg_col.y) * k, (win_bg_col.z * win_bg_col.w + title_bg_col.z) * k, new_a);
    }

    bool ResetStyle(int styleEnum, ImGuiStyle& style)
    {
        style = ImGuiStyle();
        switch (styleEnum)
        {
        case ImGuiStyle_Gray:
            style.AntiAliasedLines = true;
            style.AntiAliasedShapes = true;
            style.CurveTessellationTol = 1.25f;
            style.Alpha = 1.0f;
            //style.WindowFillAlphaDefault = .7f;

            style.WindowPadding = ImVec2(8, 8);
            style.WindowRounding = 6;
            style.ChildWindowRounding = 0;
            style.FramePadding = ImVec2(3, 3);
            style.FrameRounding = 2;
            style.ItemSpacing = ImVec2(8, 4);
            style.ItemInnerSpacing = ImVec2(5, 5);
            style.TouchExtraPadding = ImVec2(0, 0);
            style.IndentSpacing = 22;
            style.ScrollbarSize = 16;
            style.ScrollbarRounding = 4;
            style.GrabMinSize = 8;
            style.GrabRounding = 0;

            style.Colors[ImGuiCol_Text] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.16f, 0.16f, 0.18f, 0.70f);
            style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.33f, 0.29f, 0.33f, 0.60f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.39f, 0.26f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
            style.Colors[ImGuiCol_TitleBg] = ImGui::ConvertTitleBgColFromPrevVersion(style.Colors[ImGuiCol_WindowBg], ImVec4(0.26f, 0.27f, 0.74f, 1.00f));
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.28f, 0.28f, 0.76f, 0.16f);
            style.Colors[ImGuiCol_TitleBgActive] = ImGui::ConvertTitleBgColFromPrevVersion(style.Colors[ImGuiCol_WindowBg], ImVec4(0.50f, 0.50f, 1.00f, 0.55f));
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.40f, 0.55f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.18f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.67f, 0.58f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.83f, 0.88f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 1.00f, 0.67f, 1.00f);
            style.Colors[ImGuiCol_ComboBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.29f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.29f, 0.61f, 1.00f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.40f, 0.68f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.52f, 0.81f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.11f, 0.37f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.40f, 0.50f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.51f, 0.63f, 0.27f, 1.00f);
            style.Colors[ImGuiCol_Column] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.60f, 0.40f, 0.40f, 1.00f);
            style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 0.33f, 0.38f, 0.37f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 0.73f, 0.69f, 0.41f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 0.75f, 0.90f);
            style.Colors[ImGuiCol_CloseButton] = ImVec4(0.73f, 0.20f, 0.00f, 0.68f);
            style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(1.00f, 0.27f, 0.27f, 0.50f);
            style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.38f, 0.23f, 0.12f, 0.50f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.73f, 0.68f, 0.65f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 0.66f, 0.34f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.90f);
            style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
            break;

        case ImGuiStyle_Light:
            style.AntiAliasedLines = true;
            style.AntiAliasedShapes = true;
            style.CurveTessellationTol = 1.25f;
            style.Alpha = 1.0f;
            //style.WindowFillAlphaDefault = .7f;

            style.WindowPadding = ImVec2(8, 8);
            style.WindowRounding = 6;
            style.ChildWindowRounding = 0;
            style.FramePadding = ImVec2(4, 3);
            style.FrameRounding = 0;
            style.ItemSpacing = ImVec2(8, 4);
            style.ItemInnerSpacing = ImVec2(4, 4);
            style.TouchExtraPadding = ImVec2(0, 0);
            style.IndentSpacing = 21;
            style.ScrollbarSize = 16;
            style.ScrollbarRounding = 4;
            style.GrabMinSize = 8;
            style.GrabRounding = 0;

            style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.00f, 0.00f, 0.71f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
            style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.99f, 1.00f, 0.71f, 0.10f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.51f, 0.63f, 0.63f, 0.92f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.14f, 0.14f, 0.14f, 0.51f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.86f, 0.86f, 0.86f, 0.51f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.54f, 0.67f, 0.67f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.61f, 0.74f, 0.75f, 1.00f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.82f, 0.82f, 1.00f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.54f, 0.54f, 0.24f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.54f, 0.54f, 0.24f, 0.39f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.68f, 0.69f, 0.30f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.50f, 0.57f, 0.73f, 0.92f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.26f, 0.29f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.61f, 0.60f, 0.26f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.73f, 0.72f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.82f, 0.82f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_ComboBg] = ImVec4(0.51f, 0.63f, 0.63f, 0.92f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.86f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.81f, 0.82f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.87f, 0.88f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.41f, 0.59f, 0.31f, 1.00f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.65f, 0.34f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.73f, 0.38f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.42f, 0.47f, 0.88f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.51f, 0.93f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.50f, 0.62f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_Column] = ImVec4(0.13f, 0.14f, 0.11f, 1.00f);
            style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.73f, 0.75f, 0.61f, 1.00f);
            style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.89f, 0.90f, 0.70f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.61f, 0.22f, 0.22f, 1.00f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.69f, 0.24f, 0.24f, 1.00f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.80f, 0.28f, 0.28f, 1.00f);
            style.Colors[ImGuiCol_CloseButton] = ImVec4(0.67f, 0.00f, 0.00f, 0.50f);
            style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.78f, 0.00f, 0.00f, 0.60f);
            style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.92f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.17f, 0.35f, 0.03f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.41f, 0.81f, 0.06f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.81f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.48f, 0.61f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.39f, 0.12f, 0.12f, 0.20f);
            break;

        case ImGuiStyle_OSX:
        case ImGuiStyle_OSXInverse:
            // Posted by @itamago here: https://github.com/ocornut/imgui/pull/511
            style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            //style.Colors[ImGuiCol_TextHovered]           = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            //style.Colors[ImGuiCol_TextActive]            = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.7f);
            style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
            style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Column] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
            style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
            style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
            if (styleEnum == ImGuiStyle_OSXInverse)
            {
                InvertStyleColors(style);
            }

            break;

        case ImGuiStyle_DarkOpaque:
        case ImGuiStyle_DarkOpaqueInverse:
            style.AntiAliasedLines = true;
            style.AntiAliasedShapes = true;
            style.CurveTessellationTol = 1.25f;
            style.Alpha = 1.0f;
            //style.WindowFillAlphaDefault = .7f;

            style.WindowPadding = ImVec2(8, 8);
            style.WindowRounding = 4;
            style.ChildWindowRounding = 0;
            style.FramePadding = ImVec2(3, 3);
            style.FrameRounding = 0;
            style.ItemSpacing = ImVec2(8, 4);
            style.ItemInnerSpacing = ImVec2(5, 5);
            style.TouchExtraPadding = ImVec2(0, 0);
            style.IndentSpacing = 22;
            style.ScrollbarSize = 16;
            style.ScrollbarRounding = 8;
            style.GrabMinSize = 8;
            style.GrabRounding = 0;

            style.Colors[ImGuiCol_Text] = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.73f, 0.73f, 0.73f, 0.39f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.01f, 0.04f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.04f, 0.04f, 0.04f, 0.51f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.39f, 0.39f, 0.40f, 1.00f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.54f, 0.54f, 0.55f, 1.00f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.25f, 0.25f, 0.24f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.25f, 0.25f, 0.24f, 0.23f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.35f, 0.35f, 0.34f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.38f, 0.38f, 0.45f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.27f, 0.30f, 0.60f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.64f, 0.64f, 0.80f, 0.59f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.64f, 0.64f, 0.80f, 0.78f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.64f, 0.64f, 0.80f, 1.00f);
            style.Colors[ImGuiCol_ComboBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.42f, 0.42f, 0.43f, 1.00f);
            style.Colors[ImGuiCol_Column] = ImVec4(0.84f, 0.84f, 0.84f, 0.90f);
            style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.90f, 0.90f, 0.90f, 0.95f);
            style.Colors[ImGuiCol_ColumnActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
            style.Colors[ImGuiCol_CloseButton] = ImVec4(0.70f, 0.72f, 0.71f, 1.00f);
            style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.83f, 0.86f, 0.84f, 1.00f);
            style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.78f, 0.37f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.78f, 0.37f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.77f, 0.41f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.26f, 0.63f, 1.00f);
            style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

            if (styleEnum == ImGuiStyle_DarkOpaqueInverse)
            {
                InvertStyleColors(style);
                style.Colors[ImGuiCol_PopupBg] = ImVec4(0.99f, 0.96f, 1.00f, 1.00f);
            }

            break;

        case ImGuiStyle_OSXOpaque:
        case ImGuiStyle_OSXOpaqueInverse:
            //ImVec4 Full = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            style.FrameRounding = 3.0f;
            style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
            style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
            style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Column] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_CloseButton] = ImVec4(0.59f, 0.59f, 0.59f, 0.50f);
            style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
            style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

            if (styleEnum == ImGuiStyle_OSXOpaqueInverse)
            {
                InvertStyleColors(style);
                //style.Colors[ImGuiCol_PopupBg]	     = ImVec4(0.3f, 0.3f, 0.4f, 1.00f);
            }

            break;

        case ImGuiStyle_Soft:
            // style by olekristensen [https://github.com/ocornut/imgui/issues/539]
            /* olekristensen used it wth these fonts:
            io.Fonts->Clear();
            io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Light.ttf", true).c_str(), 16);
            io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Regular.ttf", true).c_str(), 16);
            io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Light.ttf", true).c_str(), 32);
            io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Regular.ttf", true).c_str(), 11);
            io.Fonts->AddFontFromFileTTF(ofToDataPath("fonts/OpenSans-Bold.ttf", true).c_str(), 11);
            io.Fonts->Build();*/

            style.WindowPadding = ImVec2(15, 15);
            style.WindowRounding = 5.0f;
            style.FramePadding = ImVec2(5, 5);
            style.FrameRounding = 4.0f;
            style.ItemSpacing = ImVec2(12, 8);
            style.ItemInnerSpacing = ImVec2(8, 6);
            style.IndentSpacing = 25.0f;
            style.ScrollbarSize = 15.0f;
            style.ScrollbarRounding = 9.0f;
            style.GrabMinSize = 5.0f;
            style.GrabRounding = 3.0f;

            style.Colors[ImGuiCol_Text] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.39f, 0.38f, 0.77f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.92f, 0.91f, 0.88f, 0.70f);
            style.Colors[ImGuiCol_ChildWindowBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.58f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.92f, 0.91f, 0.88f, 0.92f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.84f, 0.83f, 0.80f, 0.65f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.99f, 1.00f, 0.40f, 0.78f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.00f, 0.98f, 0.95f, 0.47f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 0.00f, 0.00f, 0.21f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.90f, 0.91f, 0.00f, 0.78f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_ComboBg] = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 1.00f, 0.00f, 0.80f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.99f, 1.00f, 0.22f, 0.86f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.25f, 1.00f, 0.00f, 0.76f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 1.00f, 0.00f, 0.86f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_Column] = ImVec4(0.00f, 0.00f, 0.00f, 0.32f);
            style.Colors[ImGuiCol_ColumnHovered] = ImVec4(0.25f, 1.00f, 0.00f, 0.78f);
            style.Colors[ImGuiCol_ColumnActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 1.00f, 0.00f, 0.78f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_CloseButton] = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
            style.Colors[ImGuiCol_CloseButtonHovered] = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
            style.Colors[ImGuiCol_CloseButtonActive] = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
            style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
            break;

        default:
            break;
        };

        return true;
    }
}; // namespace ImGui

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Core, HWND)
            , public Application
            , public Plugin::Core
        {
        public:
            struct Command
            {
                String function;
                std::vector<String> parameterList;
            };

        private:
            HWND window;
            bool windowActive = false;
            bool engineRunning = false;

            int currentDisplayMode = 0;
			int previousDisplayMode = 0;
            Video::DisplayModeList displayModeList;
            std::vector<StringUTF8> displayModeStringList;
            bool fullScreen = false;

           
            JSON::Object configuration;
            ShuntingYard shuntingYard;

            Timer timer;
            float mouseSensitivity = 0.5f;

            Video::DevicePtr videoDevice;
            Plugin::RendererPtr renderer;
            Engine::ResourcesPtr resources;
            std::list<Plugin::ProcessorPtr> processorList;
            Plugin::PopulationPtr population;

            struct Resources
            {
                Video::ObjectPtr vertexProgram;
                Video::ObjectPtr inputLayout;
                Video::BufferPtr constantBuffer;
                Video::ObjectPtr pixelProgram;
                Video::ObjectPtr blendState;
                Video::ObjectPtr renderState;
                Video::ObjectPtr depthState;
                Video::TexturePtr fontTexture;
                Video::ObjectPtr samplerState;
                Video::BufferPtr vertexBuffer;
                Video::BufferPtr indexBuffer;
                Video::TexturePtr logoTexture;
            };

            std::unique_ptr<Resources> gui = std::make_unique<Resources>();

            bool showCursor = false;
            bool showOptionsMenu = false;
			bool showModeChange = false;
			float modeChangeTimer = 0.0f;
            bool editorActive = false;

        public:
            Core(Context *context, HWND window)
                : ContextRegistration(context)
                , window(window)
            {
                GEK_REQUIRE(window);

                try
                {
                    configuration = JSON::Load(getContext()->getFileName(L"config.json"));
                }
                catch (const std::exception &)
                {
                    log(L"Core", Plugin::Core::LogType::Debug, L"Configuration not found, using default values");
                };

                if (!configuration.is_object())
                {
                    configuration = JSON::Object();
                }

                if (!configuration.has_member(L"display") || !configuration.get(L"display").has_member(L"mode"))
                {
                    configuration[L"display"][L"mode"] = 0;
                }

                previousDisplayMode = currentDisplayMode = configuration[L"display"][L"mode"].as_uint();

                HRESULT resultValue = CoInitialize(nullptr);
                if (FAILED(resultValue))
                {
                    throw InitializationFailed("Failed call to CoInitialize");
                }

                Video::DeviceDescription deviceDescription;
                videoDevice = getContext()->createClass<Video::Device>(L"Default::Device::Video", window, deviceDescription);
                displayModeList = videoDevice->getDisplayModeList(deviceDescription.displayFormat);
                for (auto &displayMode : displayModeList)
                {
                    StringUTF8 displayModeString(StringUTF8::Format("%vx%v, %vhz", displayMode.width, displayMode.height, uint32_t(std::ceil(float(displayMode.refreshRate.numerator) / float(displayMode.refreshRate.denominator)))));
                    switch (displayMode.aspectRatio)
                    {
                    case Video::DisplayMode::AspectRatio::_4x3:
                        displayModeString.append(" (4x3)");
                        break;
                    case Video::DisplayMode::AspectRatio::_16x9:
                        displayModeString.append(" (16x9)");
                        break;
                    case Video::DisplayMode::AspectRatio::_16x10:
                        displayModeString.append(" (16x10)");
                        break;
                    };

                    displayModeStringList.push_back(displayModeString);
                }

                setDisplayMode(currentDisplayMode);
                population = getContext()->createClass<Plugin::Population>(L"Engine::Population", (Plugin::Core *)this);
                resources = getContext()->createClass<Engine::Resources>(L"Engine::Resources", (Plugin::Core *)this, videoDevice.get());
                renderer = getContext()->createClass<Plugin::Renderer>(L"Engine::Renderer", (Plugin::Core *)this, videoDevice.get(), getPopulation(), resources.get());
                getContext()->listTypes(L"ProcessorType", [&](const wchar_t *className) -> void
                {
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(className, (Plugin::Core *)this));
                });

                onInitialized.emit();

                String baseFileName(getContext()->getFileName(L"data\\gui"));
                gui->logoTexture = videoDevice->loadTexture(FileSystem::GetFileName(baseFileName, L"logo.png"), 0);

                ImGuiIO &imGuiIo = ImGui::GetIO();
                imGuiIo.Fonts->AddFontDefault();
                imGuiIo.Fonts->Build();

                imGuiIo.KeyMap[ImGuiKey_Tab] = VK_TAB;
                imGuiIo.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
                imGuiIo.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
                imGuiIo.KeyMap[ImGuiKey_UpArrow] = VK_UP;
                imGuiIo.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
                imGuiIo.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
                imGuiIo.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
                imGuiIo.KeyMap[ImGuiKey_Home] = VK_HOME;
                imGuiIo.KeyMap[ImGuiKey_End] = VK_END;
                imGuiIo.KeyMap[ImGuiKey_Delete] = VK_DELETE;
                imGuiIo.KeyMap[ImGuiKey_Backspace] = VK_BACK;
                imGuiIo.KeyMap[ImGuiKey_Enter] = VK_RETURN;
                imGuiIo.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
                imGuiIo.KeyMap[ImGuiKey_A] = 'A';
                imGuiIo.KeyMap[ImGuiKey_C] = 'C';
                imGuiIo.KeyMap[ImGuiKey_V] = 'V';
                imGuiIo.KeyMap[ImGuiKey_X] = 'X';
                imGuiIo.KeyMap[ImGuiKey_Y] = 'Y';
                imGuiIo.KeyMap[ImGuiKey_Z] = 'Z';
                imGuiIo.ImeWindowHandle = window;

                ImGuiStyle& style = ImGui::GetStyle();
                //ImGui::SetupImGuiStyle(false, 0.9f);
                ImGui::ResetStyle(ImGui::ImGuiStyle_OSX, style);
                style.WindowTitleAlign = ImGuiAlign_Center | ImGuiAlign_VCenter;
                style.WindowRounding = 0.0f;
                style.FrameRounding = 1.0f;

                static const wchar_t *vertexShader =
                    L"cbuffer vertexBuffer : register(b0)" \
                    L"{" \
                    L"    float4x4 ProjectionMatrix;" \
                    L"};" \
                    L"" \
                    L"struct VertexInput" \
                    L"{" \
                    L"    float2 position : POSITION;" \
                    L"    float4 color : COLOR0;" \
                    L"    float2 texCoord  : TEXCOORD0;" \
                    L"};" \
                    L"" \
                    L"struct PixelOutput" \
                    L"{" \
                    L"    float4 position : SV_POSITION;" \
                    L"    float4 color : COLOR0;" \
                    L"    float2 texCoord  : TEXCOORD0;" \
                    L"};" \
                    L"" \
                    L"PixelOutput main(in VertexInput input)" \
                    L"{" \
                    L"    PixelOutput output;" \
                    L"    output.position = mul(ProjectionMatrix, float4(input.position.xy, 0.0f, 1.0f));" \
                    L"    output.color = input.color;" \
                    L"    output.texCoord  = input.texCoord;" \
                    L"    return output;" \
                    L"}";

                auto &compiled = resources->compileProgram(Video::PipelineType::Vertex, L"uiVertexProgram", L"main", vertexShader);
                gui->vertexProgram = videoDevice->createProgram(Video::PipelineType::Vertex, compiled.data(), compiled.size());
                gui->vertexProgram->setName(L"core:vertexProgram");

                std::vector<Video::InputElement> elementList;

                Video::InputElement element;
                element.format = Video::Format::R32G32_FLOAT;
                element.semantic = Video::InputElement::Semantic::Position;
                elementList.push_back(element);

                element.format = Video::Format::R32G32_FLOAT;
                element.semantic = Video::InputElement::Semantic::TexCoord;
                elementList.push_back(element);

                element.format = Video::Format::R8G8B8A8_UNORM;
                element.semantic = Video::InputElement::Semantic::Color;
                elementList.push_back(element);

                gui->inputLayout = videoDevice->createInputLayout(elementList, compiled.data(), compiled.size());
                gui->inputLayout->setName(L"core:inputLayout");

                Video::BufferDescription constantBufferDescription;
                constantBufferDescription.stride = sizeof(Math::Float4x4);
                constantBufferDescription.count = 1;
                constantBufferDescription.type = Video::BufferDescription::Type::Constant;
                gui->constantBuffer = videoDevice->createBuffer(constantBufferDescription);
                gui->constantBuffer->setName(L"core:constantBuffer");

                static const wchar_t *pixelShader =
                    L"struct PixelInput" \
                    L"{" \
                    L"    float4 position : SV_POSITION;" \
                    L"    float4 color : COLOR0;" \
                    L"    float2 texCoord  : TEXCOORD0;" \
                    L"};" \
                    L"" \
                    L"sampler pointSampler;" \
                    L"Texture2D<float4> uiTexture : register(t0);" \
                    L"" \
                    L"float4 main(PixelInput input) : SV_Target" \
                    L"{" \
                    L"    return (input.color * uiTexture.Sample(pointSampler, input.texCoord));" \
                    L"}";

                compiled = resources->compileProgram(Video::PipelineType::Pixel, L"uiPixelProgram", L"main", pixelShader);
                gui->pixelProgram = videoDevice->createProgram(Video::PipelineType::Pixel, compiled.data(), compiled.size());
                gui->pixelProgram->setName(L"core:pixelProgram");

                Video::UnifiedBlendStateInformation blendStateInformation;
                blendStateInformation.enable = true;
                blendStateInformation.colorSource = Video::BlendStateInformation::Source::SourceAlpha;
                blendStateInformation.colorDestination = Video::BlendStateInformation::Source::InverseSourceAlpha;
                blendStateInformation.colorOperation = Video::BlendStateInformation::Operation::Add;
                blendStateInformation.alphaSource = Video::BlendStateInformation::Source::InverseSourceAlpha;
                blendStateInformation.alphaDestination = Video::BlendStateInformation::Source::Zero;
                blendStateInformation.alphaOperation = Video::BlendStateInformation::Operation::Add;
                gui->blendState = videoDevice->createBlendState(blendStateInformation);
                gui->blendState->setName(L"core:blendState");

                Video::RenderStateInformation renderStateInformation;
                renderStateInformation.fillMode = Video::RenderStateInformation::FillMode::Solid;
                renderStateInformation.cullMode = Video::RenderStateInformation::CullMode::None;
                renderStateInformation.scissorEnable = true;
                renderStateInformation.depthClipEnable = true;
                gui->renderState = videoDevice->createRenderState(renderStateInformation);
                gui->renderState->setName(L"core:renderState");

                Video::DepthStateInformation depthStateInformation;
                depthStateInformation.enable = true;
                depthStateInformation.comparisonFunction = Video::ComparisonFunction::LessEqual;
                depthStateInformation.writeMask = Video::DepthStateInformation::Write::Zero;
                gui->depthState = videoDevice->createDepthState(depthStateInformation);
                gui->depthState->setName(L"core:depthState");

                uint8_t *pixels = nullptr;
                int32_t fontWidth = 0, fontHeight = 0;
                imGuiIo.Fonts->GetTexDataAsRGBA32(&pixels, &fontWidth, &fontHeight);

                Video::TextureDescription fontDescription;
                fontDescription.format = Video::Format::R8G8B8A8_UNORM;
                fontDescription.width = fontWidth;
                fontDescription.height = fontHeight;
                fontDescription.flags = Video::TextureDescription::Flags::Resource;
                gui->fontTexture = videoDevice->createTexture(fontDescription, pixels);

                imGuiIo.Fonts->TexID = (Video::Object *)gui->fontTexture.get();

                Video::SamplerStateInformation samplerStateInformation;
                samplerStateInformation.filterMode = Video::SamplerStateInformation::FilterMode::MinificationMagnificationMipMapPoint;
                samplerStateInformation.addressModeU = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerStateInformation.addressModeV = Video::SamplerStateInformation::AddressMode::Wrap;
                samplerStateInformation.addressModeW = Video::SamplerStateInformation::AddressMode::Wrap;
                gui->samplerState = videoDevice->createSamplerState(samplerStateInformation);
                gui->samplerState->setName(L"core:samplerState");

                imGuiIo.UserData = this;
                imGuiIo.RenderDrawListsFn = [](ImDrawData *drawData)
                {
                    ImGuiIO &imGuiIo = ImGui::GetIO();
                    Core *core = static_cast<Core *>(imGuiIo.UserData);
                    core->renderDrawData(drawData);
                };

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

                RAWINPUTDEVICE inputDevice;
                inputDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
                inputDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
                inputDevice.dwFlags = 0;
                inputDevice.hwndTarget = window;
                RegisterRawInputDevices(&inputDevice, 1, sizeof(RAWINPUTDEVICE));

                windowActive = true;
                engineRunning = true;

                population->load(L"demo");
            }

            ~Core(void)
            {
                ImGui::GetIO().Fonts->TexID = 0;
                ImGui::Shutdown();

                gui = nullptr;
                processorList.clear();
                renderer = nullptr;
                resources = nullptr;
                population = nullptr;
                videoDevice = nullptr;
                JSON::Save(getContext()->getFileName(L"config.json"), configuration);
                CoUninitialize();
            }

			void centerWindow(void)
			{
				RECT clientRectangle;
				GetWindowRect(window, &clientRectangle);
				int xPosition = ((GetSystemMetrics(SM_CXSCREEN) - (clientRectangle.right - clientRectangle.left)) / 2);
				int yPosition = ((GetSystemMetrics(SM_CYSCREEN) - (clientRectangle.bottom - clientRectangle.top)) / 2);
				SetWindowPos(window, nullptr, xPosition, yPosition, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}

			void setDisplayMode(uint32_t displayMode)
			{
                if (displayMode >= displayModeList.size())
                {
                    throw InvalidDisplayMode("Invalid display mode encountered");
                }

				currentDisplayMode = displayMode;
				videoDevice->setDisplayMode(displayModeList[displayMode]);
				centerWindow();
				onResize.emit();
			}

            // Plugin::Core
            void log(const wchar_t *system, LogType logType, const wchar_t *message)
            {
                OutputDebugString(String::Format(L"(%v) %v\r\n", system, message));
            }

            JSON::Object &getConfiguration(void)
            {
                return configuration;
            }

            JSON::Object const &getConfiguration(void) const
            {
                return configuration;
            }

            bool isEditorActive(void) const
            {
                return editorActive;
            }

            Plugin::Population * getPopulation(void) const
            {
                return population.get();
            }

            Plugin::Resources * getResources(void) const
            {
                return resources.get();
            }

            Plugin::Renderer * getRenderer(void) const
            {
                return renderer.get();
            }

            ImGuiContext *getDefaultUIContext(void) const
            {
                return ImGui::GetCurrentContext();
            }

            void listProcessors(std::function<void(Plugin::Processor *)> onProcessor)
            {
                for (auto &processor : processorList)
                {
                    onProcessor(processor.get());
                }
            }

            // Application
            Result windowEvent(const Event &eventData)
            {
                GEK_REQUIRE(videoDevice);
                GEK_REQUIRE(population);

                switch (eventData.message)
                {
                case WM_CLOSE:
                    engineRunning = false;
                    return 0;

                case WM_ACTIVATE:
                    if (HIWORD(eventData.wParam))
                    {
                        windowActive = false;
                    }
                    else
                    {
                        switch (LOWORD(eventData.wParam))
                        {
                        case WA_ACTIVE:
                        case WA_CLICKACTIVE:
                            windowActive = true;
                            break;

                        case WA_INACTIVE:
                            windowActive = false;
                            break;
                        };
                    }

                    return 0;

                case WM_SIZE:
                    if (eventData.wParam != SIZE_MINIMIZED)
                    {
                        videoDevice->handleResize();
                        onResize.emit();
                    }

                    return 0;
                };

                if (showCursor)
                {
                    ImGuiIO &imGuiIo = ImGui::GetIO();
                    switch (eventData.message)
                    {
                    case WM_SETCURSOR:
                        if (LOWORD(eventData.lParam) == HTCLIENT)
                        {
                            ShowCursor(false);
                            imGuiIo.MouseDrawCursor = true;
                        }
                        else
                        {
                            ShowCursor(true);
                            imGuiIo.MouseDrawCursor = false;
                        }

                        return 0;

                    case WM_LBUTTONDOWN:
                        imGuiIo.MouseDown[0] = true;
                        return 0;

                    case WM_LBUTTONUP:
                        imGuiIo.MouseDown[0] = false;
                        return 0;

                    case WM_RBUTTONDOWN:
                        imGuiIo.MouseDown[1] = true;
                        return 0;

                    case WM_RBUTTONUP:
                        imGuiIo.MouseDown[1] = false;
                        return 0;

                    case WM_MBUTTONDOWN:
                        imGuiIo.MouseDown[2] = true;
                        return 0;

                    case WM_MBUTTONUP:
                        imGuiIo.MouseDown[2] = false;
                        return 0;

                    case WM_MOUSEWHEEL:
                        imGuiIo.MouseWheel += GET_WHEEL_DELTA_WPARAM(eventData.wParam) > 0 ? +1.0f : -1.0f;
                        return 0;

                    case WM_MOUSEMOVE:
                        imGuiIo.MousePos.x = (int16_t)(eventData.lParam);
                        imGuiIo.MousePos.y = (int16_t)(eventData.lParam >> 16);
                        return 0;

                    case WM_KEYDOWN:
                        if (eventData.wParam < 256)
                        {
                            imGuiIo.KeysDown[eventData.wParam] = 1;
                        }

                        return 0;

                    case WM_KEYUP:
                        if (eventData.wParam == VK_ESCAPE)
                        {
                            imGuiIo.MouseDrawCursor = false;
                            showCursor = false;
                        }

                        if (eventData.wParam < 256)
                        {
                            imGuiIo.KeysDown[eventData.wParam] = 0;
                        }

                        return 0;

                    case WM_CHAR:
                        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
                        if (eventData.wParam > 0 && eventData.wParam < 0x10000)
                        {
                            imGuiIo.AddInputCharacter((uint16_t)eventData.wParam);
                        }

                        return 0;
                    };
                }
                else
                {
                    auto addAction = [this](WPARAM wParam, bool state) -> void
                    {
                        switch (wParam)
                        {
                        case 'W':
                        case VK_UP:
                            population->action(L"move_forward", state);
                            break;

                        case 'S':
                        case VK_DOWN:
                            population->action(L"move_backward", state);
                            break;

                        case 'A':
                        case VK_LEFT:
                            population->action(L"strafe_left", state);
                            break;

                        case 'D':
                        case VK_RIGHT:
                            population->action(L"strafe_right", state);
                            break;

                        case VK_SPACE:
                            population->action(L"jump", state);
                            break;

                        case VK_LCONTROL:
                            population->action(L"crouch", state);
                            break;
                        };
                    };

                    switch (eventData.message)
                    {
                    case WM_SETCURSOR:
                        ShowCursor(false);
                        return 0;

                    case WM_KEYDOWN:
                        addAction(eventData.wParam, true);
                        return 0;

                    case WM_KEYUP:
                        addAction(eventData.wParam, false);
                        if (eventData.wParam == VK_ESCAPE)
                        {
                            showCursor = true;
                        }

                        return 0;

                    case WM_INPUT:
                        if (population)
                        {
                            UINT inputSize = 40;
                            static BYTE rawInputBuffer[40];
                            GetRawInputData((HRAWINPUT)eventData.lParam, RID_INPUT, rawInputBuffer, &inputSize, sizeof(RAWINPUTHEADER));

                            RAWINPUT *rawInput = (RAWINPUT*)rawInputBuffer;
                            if (rawInput->header.dwType == RIM_TYPEMOUSE)
                            {
                                float xMovement = (float(rawInput->data.mouse.lLastX) * mouseSensitivity);
                                float yMovement = (float(rawInput->data.mouse.lLastY) * mouseSensitivity);
                                population->action(L"turn", xMovement);
                                population->action(L"tilt", yMovement);
                            }

                            return 0;
                        }
                    };
                }

                return Result();
            }

            bool update(void)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();

                auto backBuffer = videoDevice->getBackBuffer();
                uint32_t width = backBuffer->getDescription().width;
                uint32_t height = backBuffer->getDescription().height;
                imGuiIo.DisplaySize = ImVec2(float(width), float(height));
                float barWidth = float(width);

                timer.update();
                imGuiIo.DeltaTime = float(timer.getUpdateTime());

                // Read keyboard modifiers inputs
                imGuiIo.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                imGuiIo.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                imGuiIo.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                imGuiIo.KeySuper = false;
                // imGuiIo.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
                // imGuiIo.MousePos : filled by WM_MOUSEMOVE events
                // imGuiIo.MouseDown : filled by WM_*BUTTON* events
                // imGuiIo.MouseWheel : filled by WM_MOUSEWHEEL events

                ImGui::NewFrame();
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImGui::Begin("GEK Engine", nullptr, ImVec2(0, 0), 0.0f, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
                if (windowActive)
                {
                    onDisplay.emit();
                    onInterface.emit(showCursor);

                    auto imGuiContext = ImGui::GetCurrentContext();
                    for (auto &window : imGuiContext->Windows)
                    {
                        if (strcmp(window->Name, "Editor") == 0)
                        {
                            barWidth = (width - window->Size.x);
                        }
                    }

                    if (showCursor)
                    {
                        ImGui::SetNextWindowSize(ImVec2(barWidth, 0));
                        ImGui::SetNextWindowPos(ImVec2(0, 0));
                        ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

                        ImGui::PushItemWidth(350.0f);
                        if (ImGui::Combo("Display Mode", &currentDisplayMode, [](void *data, int index, const char **text) -> bool
                        {
                            Core *core = static_cast<Core *>(data);
                            auto &mode = core->displayModeStringList[index];
                            (*text) = mode.c_str();
                            return true;
                        }, this, displayModeStringList.size(), 5))
                        {
                            configuration[L"display"][L"mode"] = currentDisplayMode;
                            setDisplayMode(currentDisplayMode);
                            showModeChange = true;
                            modeChangeTimer = 5.0f;
                        }

                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        if (ImGui::Checkbox("FullScreen", &fullScreen))
                        {
                            if (fullScreen)
                            {
                                SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
                            }

                            videoDevice->setFullScreenState(fullScreen);
                            onResize.emit();
                            if (!fullScreen)
                            {
                                centerWindow();
                            }
                        }

                        ImGui::SameLine();
                        ImGui::Checkbox("Editor", &editorActive);
                        ImGui::End();

                        if (showModeChange)
                        {
                            ImGui::SetNextWindowPosCenter();
                            ImGui::Begin("Keep Display Mode", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
                            ImGui::Text("Keep Display Mode?");

                            if (ImGui::Button("Yes"))
                            {
                                showModeChange = false;
                                previousDisplayMode = currentDisplayMode;
                            }

                            ImGui::SameLine();
                            modeChangeTimer -= float(timer.getUpdateTime());
                            if (modeChangeTimer <= 0.0f || ImGui::Button("No"))
                            {
                                showModeChange = false;
                                setDisplayMode(previousDisplayMode);
                            }

                            ImGui::Text(StringUTF8::Format("(Revert in %v seconds)", uint32_t(modeChangeTimer)));

                            ImGui::End();
                        }

                        population->update(0.0f);
                    }
                    else
                    {
                        float frameTime = float(timer.getUpdateTime());
                        population->update(frameTime);
                    }
                }

                if (population->isLoading())
                {
                    ImGui::SetNextWindowPosCenter();
                    ImGui::Begin("GEK Engine##Loading", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoCollapse);
                    ImGui::Dummy(ImVec2(200, 0));
                    ImGui::Text("Loading...");
                    ImGui::End();
                }
                else if (!windowActive)
                {
                    ImGui::SetNextWindowPosCenter();
                    ImGui::Begin("GEK Engine##Paused", nullptr, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoCollapse);
                    ImGui::Dummy(ImVec2(200, 0));
                    ImGui::Text("Paused");
                    ImGui::End();
                }

                if (windowActive && !showCursor)
                {
                    RECT windowRectangle;
                    GetWindowRect(window, &windowRectangle);
					SetCursorPos(
                        int(Math::Interpolate(float(windowRectangle.left), float(windowRectangle.right), 0.5f)),
						int(Math::Interpolate(float(windowRectangle.top), float(windowRectangle.bottom), 0.5f)));
                }

                renderer->renderOverlay(videoDevice->getDefaultContext(), resources->getResourceHandle(L"screen"), ResourceHandle());

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
                //ImGui::SetNextWindowSize(ImVec2(barWidth, 0));
                ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 22));
                ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
                ImGui::Image((Video::Object *)gui->logoTexture.get(), ImVec2(16, 16));
                ImGui::SameLine();
                ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::End();
                ImGui::PopStyleVar();

                ImGui::End();
                ImGui::Render();
                
                videoDevice->present(false);

                return engineRunning;
            }

            // ImGui
            void renderDrawData(ImDrawData *drawData)
            {
                if (!gui->vertexBuffer || gui->vertexBuffer->getDescription().count < uint32_t(drawData->TotalVtxCount))
                {
                    Video::BufferDescription vertexBufferDescription;
                    vertexBufferDescription.stride = sizeof(ImDrawVert);
                    vertexBufferDescription.count = drawData->TotalVtxCount;
                    vertexBufferDescription.type = Video::BufferDescription::Type::Vertex;
                    vertexBufferDescription.flags = Video::BufferDescription::Flags::Mappable;
                    gui->vertexBuffer = videoDevice->createBuffer(vertexBufferDescription);
                    gui->vertexBuffer->setName(String::Format(L"core:vertexBuffer:%v", gui->vertexBuffer.get()));
                }

                if (!gui->indexBuffer || gui->indexBuffer->getDescription().count < uint32_t(drawData->TotalIdxCount))
                {
                    Video::BufferDescription vertexBufferDescription;
                    vertexBufferDescription.count = drawData->TotalIdxCount;
                    vertexBufferDescription.type = Video::BufferDescription::Type::Index;
                    vertexBufferDescription.flags = Video::BufferDescription::Flags::Mappable;
                    switch (sizeof(ImDrawIdx))
                    {
                    case 2:
                        vertexBufferDescription.format = Video::Format::R16_UINT;
                        break;

                    case 4:
                        vertexBufferDescription.format = Video::Format::R32_UINT;
                        break;

                    default:
                        throw InvalidIndexBufferFormat("Index buffer can only be 16bit or 32bit");
                    };

                    gui->indexBuffer = videoDevice->createBuffer(vertexBufferDescription);
                    gui->indexBuffer->setName(String::Format(L"core:vertexBuffer:%v", gui->indexBuffer.get()));
                }

                bool dataUploaded = false;
                ImDrawVert* vertexData = nullptr;
                ImDrawIdx* indexData = nullptr;
                if (videoDevice->mapBuffer(gui->vertexBuffer.get(), vertexData))
                {
                    if (videoDevice->mapBuffer(gui->indexBuffer.get(), indexData))
                    {
                        for (int32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
                        {
                            const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
                            std::copy(commandList->VtxBuffer.Data, (commandList->VtxBuffer.Data + commandList->VtxBuffer.Size), vertexData);
                            std::copy(commandList->IdxBuffer.Data, (commandList->IdxBuffer.Data + commandList->IdxBuffer.Size), indexData);
                            vertexData += commandList->VtxBuffer.Size;
                            indexData += commandList->IdxBuffer.Size;
                        }

                        dataUploaded = true;
                        videoDevice->unmapBuffer(gui->indexBuffer.get());
                    }

                    videoDevice->unmapBuffer(gui->vertexBuffer.get());
                }

                if (dataUploaded)
                {
                    auto backBuffer = videoDevice->getBackBuffer();
                    uint32_t width = backBuffer->getDescription().width;
                    uint32_t height = backBuffer->getDescription().height;
                    auto orthographic = Math::Float4x4::MakeOrthographic(0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f);
                    videoDevice->updateResource(gui->constantBuffer.get(), &orthographic);

                    auto videoContext = videoDevice->getDefaultContext();
                    resources->setBackBuffer(videoContext, nullptr);

                    videoContext->setInputLayout(gui->inputLayout.get());
                    videoContext->setVertexBufferList({ gui->vertexBuffer.get() }, 0);
                    videoContext->setIndexBuffer(gui->indexBuffer.get(), 0);
                    videoContext->setPrimitiveType(Video::PrimitiveType::TriangleList);
                    videoContext->vertexPipeline()->setProgram(gui->vertexProgram.get());
                    videoContext->vertexPipeline()->setConstantBufferList({ gui->constantBuffer.get() }, 0);
                    videoContext->pixelPipeline()->setProgram(gui->pixelProgram.get());
                    videoContext->pixelPipeline()->setSamplerStateList({ gui->samplerState.get() }, 0);

                    videoContext->setBlendState(gui->blendState.get(), Math::Float4::Black, 0xFFFFFFFF);
                    videoContext->setDepthState(gui->depthState.get(), 0);
                    videoContext->setRenderState(gui->renderState.get());

                    uint32_t vertexOffset = 0;
                    uint32_t indexOffset = 0;
                    for (int32_t commandListIndex = 0; commandListIndex < drawData->CmdListsCount; ++commandListIndex)
                    {
                        const ImDrawList* commandList = drawData->CmdLists[commandListIndex];
                        for (int32_t commandIndex = 0; commandIndex < commandList->CmdBuffer.Size; ++commandIndex)
                        {
                            const ImDrawCmd* command = &commandList->CmdBuffer[commandIndex];
                            if (command->UserCallback)
                            {
                                command->UserCallback(commandList, command);
                            }
                            else
                            {
                                std::vector<Math::UInt4> scissorBoxList(1);
                                scissorBoxList[0].minimum.x = uint32_t(command->ClipRect.x);
                                scissorBoxList[0].minimum.y = uint32_t(command->ClipRect.y);
                                scissorBoxList[0].maximum.x = uint32_t(command->ClipRect.z);
                                scissorBoxList[0].maximum.y = uint32_t(command->ClipRect.w);
                                videoContext->setScissorList(scissorBoxList);

                                std::vector<Video::Object *> textureList(1);
                                textureList[0] = (Video::Object *)command->TextureId;
                                videoContext->pixelPipeline()->setResourceList(textureList, 0);

                                videoContext->drawIndexedPrimitive(command->ElemCount, indexOffset, vertexOffset);
                            }

                            indexOffset += command->ElemCount;
                        }

                        vertexOffset += commandList->VtxBuffer.Size;
                    }
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Core);
    }; // namespace Implementation
}; // namespace Gek
