#pragma once

#include <vector>
#include <map>

namespace Gek
{
    enum class AspectRatio : uint8_t
    {
		Unknown = 0,
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

	struct DisplayModes
	{
		std::multimap<uint32_t, DisplayMode> allModesMap;

		DisplayModes(void);
		std::vector<DisplayMode> getBitDepth(uint32_t bitDepth);
	};
}; // namespace Gek
