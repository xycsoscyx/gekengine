/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
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
    inline size_t CombineHashes(const size_t upper, const size_t lower)
    {
        return upper ^ (lower + 0x9e3779b9 + (upper << 6) + (upper >> 2));
    }

    inline size_t GetHash(void)
    {
        return 0;
    }

    template <typename STRUCT>
    size_t GetStructHash(const STRUCT &data)
    {
        const uint8_t *rawData = (const uint8_t *)&data;
        size_t hash = GetHash(rawData[0]);
        size_t size = sizeof(STRUCT);
        for (size_t index = 1; index < size; index++)
        {
            hash = CombineHashes(hash, GetHash(rawData[index]));
        }

        return hash;
    }

    template <typename TYPE, typename... PARAMETERS>
    size_t GetHash(const TYPE &value, const PARAMETERS&... arguments)
    {
        size_t seed = std::hash<TYPE>()(value);
        size_t remainder = GetHash(arguments...);
        return CombineHashes(seed, remainder);
    }
};
