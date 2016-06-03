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

    std::map<uint32_t, std::vector<DisplayMode>> getDisplayModes(void)
    {
        uint32_t displayMode = 0;
        DEVMODE displayModeData = { 0 };
        std::map<uint32_t, std::vector<DisplayMode>> availbleModeList;
        while (EnumDisplaySettings(0, displayMode++, &displayModeData))
        {
            std::vector<DisplayMode> &currentModeList = availbleModeList[displayModeData.dmBitsPerPel];
            auto findIterator = std::find_if(currentModeList.begin(), currentModeList.end(), [&](const DisplayMode &mode) -> bool
            {
                if (mode.width != displayModeData.dmPelsWidth) return false;
                if (mode.height != displayModeData.dmPelsHeight) return false;
                return true;
            });

            if (findIterator == currentModeList.end())
            {
                currentModeList.emplace_back(displayModeData.dmPelsWidth, displayModeData.dmPelsHeight,
                    getAspectRatio(displayModeData.dmPelsWidth, displayModeData.dmPelsHeight));
            }
        };

        return availbleModeList;
    }
}; // namespace Gek
