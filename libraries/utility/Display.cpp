#include "GEK\Utility\Display.h"
#include <algorithm>
#include "GEK\Utility\Trace.h"
#include <Windows.h>

namespace Gek
{
    AspectRatio getAspectRatio(uint32_t width, uint32_t height)
    {
        const float AspectRatio4x3 = (float(int32_t((4.0f / 3.0f) * 100.0f)) / 100.0f);
        const float AspectRatio16x9 = (float(int32_t((16.0f / 9.0f) * 100.0f)) / 100.0f);
        const float AspectRatio16x10 = (float(int32_t((16.0f / 10.0f) * 100.0f)) / 100.0f);
        float aspectRatio = (float(int32_t((float(width) / float(height)) * 100.0f)) / 100.0f);
        if (aspectRatio == AspectRatio4x3)
        {
            return AspectRatio::_4x3;
        }
        else if (aspectRatio == AspectRatio16x9)
        {
            return AspectRatio::_16x9;
        }
        else if (aspectRatio == AspectRatio16x10)
        {
            return AspectRatio::_16x10;
        }
        else
        {
            return AspectRatio::None;
        }
    }

    DisplayMode::DisplayMode(uint32_t width, uint32_t height)
        : width(width)
        , height(height)
        , aspectRatio(getAspectRatio(width, height))
    {
    }

    bool DisplayMode::operator == (const DisplayMode &displayMode) const
    {
        return (width == displayMode.width && height == displayMode.height);
    }

    std::multimap<uint32_t, DisplayMode> getDisplayModes(void)
    {
        uint32_t displayMode = 0;
        DEVMODE displayModeData = { 0 };
        std::multimap<uint32_t, DisplayMode> availbleModeList;
        while (EnumDisplaySettings(0, displayMode++, &displayModeData))
        {
            DisplayMode displayMode(displayModeData.dmPelsWidth, displayModeData.dmPelsHeight);
            if (std::find_if(availbleModeList.begin(), availbleModeList.end(), [&displayModeData, &displayMode](auto &modePair) -> bool
            {
                return (modePair.first == displayModeData.dmBitsPerPel && modePair.second == displayMode);
            }) == availbleModeList.end())
            {
                availbleModeList.emplace(displayModeData.dmBitsPerPel, displayMode);
            }
        };

        return availbleModeList;
    }
}; // namespace Gek
