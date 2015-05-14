#pragma once

#include <Windows.h>
#include <vector>
#include <map>

namespace Gek
{
    namespace Display
    {
        namespace AspectRatio
        {
            enum
            {
                Unknown = -1,
                _4x3 = 0,
                _16x9 = 1,
                _16x10 = 2,
            };
        }; // namespace AspectRatios

        struct Mode
        {
            UINT32 width;
            UINT32 height;
            UINT8 aspectRatio;

            Mode(UINT32 width, UINT32 height, UINT8 aspectRatio)
                : width(width)
                , height(height)
                , aspectRatio(aspectRatio)
            {
            }
        };

        std::map<UINT32, std::vector<Mode>> getModes(void);
    }; // namespace Display
}; // namespace Gek
