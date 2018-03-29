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

namespace Gek
{
    using Hash = std::size_t;

    inline Hash CombineHashes(Hash upper, Hash lower)
    {
        return upper ^ (lower + 0x9e3779b9 + (upper << 6) + (upper >> 2));
    }

    inline Hash GetHash(void)
    {
        return 0;
    }

    template <typename TYPE, typename... PARAMETERS>
    Hash GetHash(TYPE const &value, PARAMETERS const &... arguments)
    {
        Hash seed = std::hash<TYPE>()(value);
        Hash remainder = GetHash(arguments...);
        return CombineHashes(seed, remainder);
    }
};
