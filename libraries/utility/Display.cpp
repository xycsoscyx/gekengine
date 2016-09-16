#include "GEK\Math\Common.h"
#include "GEK\Utility\Display.h"
#include "GEK\Utility\Exceptions.h"
#include <algorithm>
#include <Windows.h>

namespace Gek
{
    AspectRatio getAspectRatio(uint32_t width, uint32_t height)
    {
		const float AspectRatio4x3 = (4.0f / 3.0f);
		const float AspectRatio16x9 = (16.0f / 9.0f);
		const float AspectRatio16x10 = (16.0f / 10.0f);
		float aspectRatio = (float(width) / float(height));
        if (std::abs(aspectRatio - AspectRatio4x3) < Math::Epsilon)
        {
            return AspectRatio::_4x3;
        }
        else if (std::abs(aspectRatio - AspectRatio16x9) < Math::Epsilon)
        {
            return AspectRatio::_16x9;
        }
		else if (std::abs(aspectRatio - AspectRatio16x10) < Math::Epsilon)
        {
            return AspectRatio::_16x10;
        }
        else
        {
            return AspectRatio::Unknown;
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

	std::vector<DisplayMode> DisplayModes::getBitDepth(uint32_t bitDepth)
	{
		std::vector<DisplayMode> modesList;
		auto modesRange = allModesMap.equal_range(bitDepth);
		for (auto &modeSearch = modesRange.first; modeSearch != modesRange.second; ++modeSearch)
		{
			auto &mode = modeSearch->second;
			modesList.push_back(mode);
		}

		std::sort(modesList.begin(), modesList.end(), [&](const DisplayMode &left, const DisplayMode &right) -> bool
		{
			return ((left.width * left.height) < (right.width * right.height));
		});

		return modesList;
	}

	DisplayModes::DisplayModes(void)
    {
        uint32_t displayMode = 0;
        DEVMODE displayModeData = { 0 };
        while (EnumDisplaySettings(0, displayMode++, &displayModeData))
        {
            DisplayMode displayMode(displayModeData.dmPelsWidth, displayModeData.dmPelsHeight);
            if (std::find_if(allModesMap.begin(), allModesMap.end(), [&displayModeData, &displayMode](auto &modePair) -> bool
            {
                return (modePair.first == displayModeData.dmBitsPerPel && modePair.second == displayMode);
            }) == allModesMap.end())
            {
				allModesMap.emplace(displayModeData.dmBitsPerPel, displayMode);
            }
        };
    }
}; // namespace Gek
