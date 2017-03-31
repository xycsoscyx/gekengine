/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 347000ea4cf5625a5cd325614cb63ebc84e060aa $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Sat Oct 15 19:36:10 2016 +0000 $
#pragma once

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Resources.hpp"

namespace Gek
{
    enum class ClearType : uint8_t
    {
        Unknown = 0,
        Target,
        Float,
        UInt,
    };

    struct ClearData
    {
        ClearType type;
        Math::Float4 floats;
        Math::UInt4 integers;

        ClearData(ClearType type, WString const &data);
    };

    WString getFormatSemantic(Video::Format format);
    WString getFormatSemantic(Video::Format format, uint32_t count);

    ClearType getClearType(WString const &clearType);

    uint32_t getTextureLoadFlags(WString const &loadFlags);
    uint32_t getTextureFlags(WString const &createFlags);
    uint32_t getBufferFlags(WString const &createFlags);

    std::unordered_map<WString, WString> getAliasedMap(const JSON::Object &parent, wchar_t const * const name);
}; // namespace Gek
