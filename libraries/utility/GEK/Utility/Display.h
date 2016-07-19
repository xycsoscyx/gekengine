#pragma once

#include <vector>
#include <map>

namespace Gek
{
    enum class AspectRatio : uint8_t
    {
        None = 0,
        _4x3,
        _16x9,
        _16x10,
    };

    struct DisplayMode
    {
        uint32_t width;
        uint32_t height;
        AspectRatio aspectRatio;

        DisplayMode(uint32_t width, uint32_t height);

        bool operator == (const DisplayMode &displayMode) const;
    };

    using DisplayModeMap = std::multimap<uint32_t, DisplayMode>;
    DisplayModeMap getDisplayModes(void);
}; // namespace Gek
