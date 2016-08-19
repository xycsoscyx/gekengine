#pragma once

namespace Gek
{
    inline size_t hashCombine(const size_t upper, const size_t lower)
    {
        return upper ^ (lower + 0x9e3779b9 + (upper << 6) + (upper >> 2));
    }

    inline size_t hashCombine(void)
    {
        return 0;
    }

    template <typename TYPE, typename... PARAMETERS>
    size_t hashCombine(const TYPE &value, const PARAMETERS&... arguments)
    {
        size_t seed = std::hash<TYPE>()(value);
        size_t remainder = hashCombine(arguments...);
        return hashCombine(seed, remainder);
    }
};
