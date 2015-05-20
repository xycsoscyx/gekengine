#pragma once

#include <Windows.h>
#include <vector>
#include <map>

namespace Gek
{
    namespace Display
    {
        enum class AspectRatio : UINT8
        {
            None = 0,
            _4x3,
            _16x9,
            _16x10,
        };

        struct Mode
        {
            UINT32 width;
            UINT32 height;
            AspectRatio aspectRatio;

            Mode(UINT32 width, UINT32 height, AspectRatio aspectRatio)
                : width(width)
                , height(height)
                , aspectRatio(aspectRatio)
            {
            }
        };

        std::map<UINT32, std::vector<Mode>> getModes(void);
    }; // namespace Display
}; // namespace Gek
