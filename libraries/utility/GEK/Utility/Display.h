#pragma once

#include <Windows.h>
#include <vector>
#include <map>

namespace Gek
{
    enum class AspectRatio : UINT8
    {
        None = 0,
        _4x3,
        _16x9,
        _16x10,
    };

    struct DisplayMode
    {
        UINT32 width;
        UINT32 height;
        AspectRatio aspectRatio;

        DisplayMode(UINT32 width, UINT32 height, AspectRatio aspectRatio)
            : width(width)
            , height(height)
            , aspectRatio(aspectRatio)
        {
        }
    };

    std::map<UINT32, std::vector<DisplayMode>> getDisplayModes(void);
}; // namespace Gek
