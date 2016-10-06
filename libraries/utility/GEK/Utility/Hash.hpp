#pragma once

#include <functional>
#include <string>

namespace std
{
	template <>
	struct hash<const char *>
	{
		size_t operator()(const char *value) const
		{
			return hash<string>()(value);
		}
	};

	template <>
	struct hash<const wchar_t *>
	{
		size_t operator()(const wchar_t *value) const
		{
			return hash<wstring>()(value);
		}
	};
}; // namespace std

namespace Gek
{
    inline size_t combineHashes(const size_t upper, const size_t lower)
    {
        return upper ^ (lower + 0x9e3779b9 + (upper << 6) + (upper >> 2));
    }

    inline size_t getHash(void)
    {
        return 0;
    }

    template <typename TYPE, typename... PARAMETERS>
    size_t getHash(const TYPE &value, const PARAMETERS&... arguments)
    {
        size_t seed = std::hash<TYPE>()(value);
        size_t remainder = getHash(arguments...);
        return combineHashes(seed, remainder);
    }
};
