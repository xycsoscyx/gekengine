#pragma once

#include <vector>
#include <map>

namespace Gek
{
	class Display
	{
	public:
		enum class AspectRatio : uint8_t
		{
			Unknown = 0,
			_4x3,
			_16x9,
			_16x10,
		};

		struct Mode
		{
			uint32_t width;
			uint32_t height;
			AspectRatio aspectRatio;

			Mode(uint32_t width, uint32_t height);

			bool operator == (const Mode &mode) const;
		};

		using ModesList = std::vector<Mode>;

	private:
		std::multimap<uint32_t, Mode> modesMap;

	public:
		Display(void);

		ModesList getModes(uint32_t bitsPerPixel);
	};
}; // namespace Gek
