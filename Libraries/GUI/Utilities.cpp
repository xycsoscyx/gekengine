#include "GEK/GUI/Utilities.hpp"
#include <unordered_map>

namespace Gek
{
    namespace GUI
    {
        bool InputFloat(char const * label, float *value, float step, float stepFast, int decimalPrecision, ImGuiInputTextFlags flags)
        {
            ::ImGui::Text(label);
            return ::ImGui::InputFloat(String::Format("##%v", label).c_str(), value, step, stepFast, decimalPrecision, flags);
        }

        bool InputFloat2(char const * label, float value[2], int decimalPrecision, ImGuiInputTextFlags flags)
        {
            ::ImGui::Text(label);
            return ::ImGui::InputFloat2(String::Format("##%v", label).c_str(), value, decimalPrecision, flags);
        }

        bool InputFloat3(char const * label, float value[3], int decimalPrecision, ImGuiInputTextFlags flags)
        {
            ::ImGui::Text(label);
            return ::ImGui::InputFloat3(String::Format("##%v", label).c_str(), value, decimalPrecision, flags);
        }

        bool InputFloat4(char const * label, float value[4], int decimalPrecision, ImGuiInputTextFlags flags)
        {
            ::ImGui::Text(label);
            return ::ImGui::InputFloat4(String::Format("##%v", label).c_str(), value, decimalPrecision, flags);
        }

        std::unordered_map<std::string, std::string> labelStringMap;
        bool InputString(char const * label, std::string &string, ImGuiInputTextFlags flags, ImGuiTextEditCallback callback, void *userData)
        {
            ::ImGui::Text(label);
            auto &internalSearch = labelStringMap.insert(std::make_pair(label, string));
            auto &internalString = internalSearch.first->second;
            if (internalSearch.second)
            {
                internalString.reserve(256);
            }

            if (::ImGui::InputText(String::Format("##%v", label).c_str(), &internalString.front(), 255, flags, callback, userData))
            {
                string = internalString;
                return true;
            }

            return false;

        }

        bool ListBox(char const * label, int *currentSelectionIndex, bool(*itemDataCallback)(void *userData, int index, const char ** textOutput), void *userData, int itemCount, int visibleItemCount)
        {
            ::ImGui::Text(label);
            return ::ImGui::ListBox(String::Format("##%v", label).c_str(), currentSelectionIndex, itemDataCallback, userData, itemCount, visibleItemCount);
        }

        void PlotEx(ImGuiPlotType plot_type, char const * label, float(*itemDataCallback)(void *userData, int index), void *userData, int itemCount, int itemStartIndex, float scaleMinimum, float scaleMaximum, ImVec2 graphSize)
        {
            ImGuiWindow *window = ::ImGui::GetCurrentWindow();
            if (window->SkipItems)
            {
                return;
            }

            ImGuiContext &guiContext = *GImGui;
            const ImGuiStyle &style = guiContext.Style;
            const ImVec2 labelSize = ::ImGui::CalcTextSize(label, nullptr, true);
            if (graphSize.x == 0.0f)
            {
                graphSize.x = ::ImGui::CalcItemWidth();
            }

            if (graphSize.y == 0.0f)
            {
                graphSize.y = labelSize.y + (style.FramePadding.y * 2);
            }

            const ImRect frameBoundingBox(window->DC.CursorPos, window->DC.CursorPos + ImVec2(graphSize.x, graphSize.y));
            const ImRect clientBoundingBox(frameBoundingBox.Min + style.FramePadding, frameBoundingBox.Max - style.FramePadding);
            const ImRect totalBoundingBox(frameBoundingBox.Min, frameBoundingBox.Max + ImVec2(labelSize.x > 0.0f ? style.ItemInnerSpacing.x + labelSize.x : 0.0f, 0));
            ::ImGui::ItemSize(totalBoundingBox, style.FramePadding.y);
            if (!::ImGui::ItemAdd(totalBoundingBox, nullptr))
            {
                return;
            }

            // Determine scale from values if not specified
            if (scaleMinimum == std::numeric_limits<float>::max() || scaleMaximum == std::numeric_limits<float>::max())
            {
                float valueMinimum = std::numeric_limits<float>::max();
                float valueMaximum = -std::numeric_limits<float>::max();
                for (int item = 0; item < itemCount; ++item)
                {
                    const float value = itemDataCallback(userData, item);
                    valueMinimum = ImMin(valueMinimum, value);
                    valueMaximum = ImMax(valueMaximum, value);
                }

                if (scaleMinimum == std::numeric_limits<float>::max())
                {
                    scaleMinimum = valueMinimum;
                }

                if (scaleMaximum == std::numeric_limits<float>::max())
                {
                    scaleMaximum = valueMaximum;
                }
            }

            ::ImGui::RenderFrame(frameBoundingBox.Min, frameBoundingBox.Max, ::ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
            if (itemCount > 0)
            {
                int resultsWidth = ImMin((int)graphSize.x, itemCount) + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0);
                int adjustedItemCount = itemCount + ((plot_type == ImGuiPlotType_Lines) ? -1 : 0);

                // Tooltip on hover
                int hoveredIndex = -1;
                if (::ImGui::IsHovered(clientBoundingBox, 0))
                {
                    const float mousePosition = ImClamp((guiContext.IO.MousePos.x - clientBoundingBox.Min.x) / (clientBoundingBox.Max.x - clientBoundingBox.Min.x), 0.0f, 0.9999f);
                    const int mouseIndex = (int)(mousePosition * adjustedItemCount);
                    IM_ASSERT(mouseIndex >= 0 && mouseIndex < itemCount);

                    const float valueLeft = itemDataCallback(userData, (mouseIndex + itemStartIndex) % itemCount);
                    const float valueRight = itemDataCallback(userData, (mouseIndex + 1 + itemStartIndex) % itemCount);
                    if (plot_type == ImGuiPlotType_Lines)
                    {
                        ::ImGui::SetTooltip("%8.4g - %8.4g", valueLeft, valueRight);
                    }
                    else if (plot_type == ImGuiPlotType_Histogram)
                    {
                        ::ImGui::SetTooltip("%8.4g", valueLeft);
                    }

                    hoveredIndex = mouseIndex;
                }

                const float timeStep = 1.0f / (float)resultsWidth;

                float valueLeft = itemDataCallback(userData, (0 + itemStartIndex) % itemCount);
                float timeLeft = 0.0f;
                ImVec2 nornalizedTimeLeft = ImVec2(timeLeft, 1.0f - ImSaturate((valueLeft - scaleMinimum) / (scaleMaximum - scaleMinimum)));    // Point in the normalized space of our target rectangle

                const ImU32 colorBase = ::ImGui::GetColorU32((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLines : ImGuiCol_PlotHistogram);
                const ImU32 colorHovered = ::ImGui::GetColorU32((plot_type == ImGuiPlotType_Lines) ? ImGuiCol_PlotLinesHovered : ImGuiCol_PlotHistogramHovered);

                for (int result = 0; result < resultsWidth; ++result)
                {
                    const float timeRight = timeLeft + timeStep;
                    const int valueRightIndex = (int)(timeLeft * adjustedItemCount + 0.5f);
                    IM_ASSERT(valueRightIndex >= 0 && valueRightIndex < itemCount);
                    const float valueRight = itemDataCallback(userData, (valueRightIndex + itemStartIndex + 1) % itemCount);
                    const ImVec2 normalizedTimeRight = ImVec2(timeRight, 1.0f - ImSaturate((valueRight - scaleMinimum) / (scaleMaximum - scaleMinimum)));

                    // NB: Draw calls are merged together by the DrawList system. Still, we should render our batch are lower level to save a bit of CPU.
                    ImVec2 positionLeft = ImLerp(clientBoundingBox.Min, clientBoundingBox.Max, nornalizedTimeLeft);
                    ImVec2 positionRight = ImLerp(clientBoundingBox.Min, clientBoundingBox.Max, (plot_type == ImGuiPlotType_Lines) ? normalizedTimeRight : ImVec2(normalizedTimeRight.x, 1.0f));
                    if (plot_type == ImGuiPlotType_Lines)
                    {
                        window->DrawList->AddLine(positionLeft, positionRight, hoveredIndex == valueRightIndex ? colorHovered : colorBase);
                    }
                    else if (plot_type == ImGuiPlotType_Histogram)
                    {
                        if (positionRight.x >= positionLeft.x + 2.0f)
                        {
                            positionRight.x -= 1.0f;
                        }

                        window->DrawList->AddRectFilled(positionLeft, positionRight, hoveredIndex == valueRightIndex ? colorHovered : colorBase);
                    }

                    timeLeft = timeRight;
                    nornalizedTimeLeft = normalizedTimeRight;
                }
            }

            auto scaleSize = (scaleMaximum - scaleMinimum);
            for (float scale = 0.0f; scale <= 1.0f; scale += 0.2f)
            {
                ::ImGui::RenderTextClipped(
                    ImVec2(frameBoundingBox.Min.x + style.FramePadding.x, frameBoundingBox.Min.y + style.FramePadding.y),
                    ImVec2(frameBoundingBox.Max.x + style.FramePadding.x, frameBoundingBox.Max.y - style.FramePadding.y),
                    String::Format("%v", ((scaleSize * scale) + scaleMinimum)).c_str(), nullptr, nullptr, ImVec2(0.0f, (1.0f - scale)));
            }
        }

        void PlotLines(char const * label, float(*itemDataCallback)(void *userData, int index), void *userData, int itemCount, int itemStartIndex, float scaleMinimum, float scaleMaximum, ImVec2 graphSize)
        {
            PlotEx(ImGuiPlotType_Lines, label, itemDataCallback, userData, itemCount, itemStartIndex, scaleMinimum, scaleMaximum, graphSize);
        }

        void PlotHistogram(char const * label, float(*itemDataCallback)(void *userData, int index), void *userData, int itemCount, int itemStartIndex, float scaleMinimum, float scaleMaximum, ImVec2 graphSize)
        {
            PlotEx(ImGuiPlotType_Histogram, label, itemDataCallback, userData, itemCount, itemStartIndex, scaleMinimum, scaleMaximum, graphSize);
        }
    }; // namespace GUI
}; // namespace Gek
