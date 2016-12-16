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

    template <typename TYPE, typename... PARAMETERS>
    size_t GetHash(const TYPE &value, const PARAMETERS&... arguments)
    {
        size_t seed = std::hash<TYPE>()(value);
        size_t remainder = GetHash(arguments...);
        return CombineHashes(seed, remainder);
    }

    template <typename STRUCT>
    size_t GetStructHash(const STRUCT &data)
    {
        size_t hash = 0;

        size_t remainder = (sizeof(STRUCT) % 4);
        const uint8_t *rawBytes = (const uint8_t *)&data;
        for (size_t index = 0; index < remainder; index++)
        {
            hash = CombineHashes(hash, GetHash(rawBytes++));
        }

        size_t size = (sizeof(STRUCT) / 4);
        const uint32_t *rawIntegers = (const uint32_t *)rawBytes;
        for (size_t index = 0; index < size; index++)
        {
            hash = CombineHashes(hash, GetHash(rawIntegers++));
        }

        return hash;
    }
};
