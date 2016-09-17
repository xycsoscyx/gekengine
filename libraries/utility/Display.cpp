#include "GEK\Math\Common.h"
#include "GEK\Utility\Display.h"
#include "GEK\Utility\Exceptions.h"
#include <algorithm>
#include <Windows.h>

namespace Gek
{
    Display::AspectRatio getAspectRatio(uint32_t width, uint32_t height)
    {
		const float AspectRatio4x3 = (4.0f / 3.0f);
		const float AspectRatio16x9 = (16.0f / 9.0f);
		const float AspectRatio16x10 = (16.0f / 10.0f);
		float aspectRatio = (float(width) / float(height));
        if (std::abs(aspectRatio - AspectRatio4x3) < Math::Epsilon)
        {
            return Display::AspectRatio::_4x3;
        }
        else if (std::abs(aspectRatio - AspectRatio16x9) < Math::Epsilon)
        {
            return Display::AspectRatio::_16x9;
        }
		else if (std::abs(aspectRatio - AspectRatio16x10) < Math::Epsilon)
        {
            return Display::AspectRatio::_16x10;
        }
        else
        {
            return Display::AspectRatio::Unknown;
        }
    }

	Display::Mode::Mode(uint32_t width, uint32_t height)
        : width(width)
        , height(height)
        , aspectRatio(getAspectRatio(width, height))
    {
    }

    bool Display::Mode::operator == (const Display::Mode &mode) const
    {
        return (width == mode.width && height == mode.height);
    }

	Display::Display(void)
    {
        uint32_t displayMode = 0;
        DEVMODE deviceMode = { 0 };
        while (EnumDisplaySettings(0, displayMode++, &deviceMode))
        {
			Mode mode(deviceMode.dmPelsWidth, deviceMode.dmPelsHeight);
            if (std::find_if(modesMap.begin(), modesMap.end(), [&deviceMode, &mode](auto &modeSearch) -> bool
            {
                return (modeSearch.first == deviceMode.dmBitsPerPel && modeSearch.second == mode);
            }) == modesMap.end())
            {
				modesMap.emplace(deviceMode.dmBitsPerPel, mode);
            }
        };
    }

	Display::ModesList Display::getModes(uint32_t bitsPerPixel)
	{
		ModesList modesList;
		auto modesRange = modesMap.equal_range(bitsPerPixel);
		for (auto &modeSearch = modesRange.first; modeSearch != modesRange.second; ++modeSearch)
		{
			auto &mode = modeSearch->second;
			modesList.push_back(mode);
		}

		std::sort(modesList.begin(), modesList.end(), [&](const Mode &left, const Mode &right) -> bool
		{
			return ((left.width * left.height) < (right.width * right.height));
		});

		return modesList;
	}
}; // namespace Gek
