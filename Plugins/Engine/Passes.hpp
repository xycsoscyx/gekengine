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

        ClearData(ClearType type, const String &data);
    };

    String getFormatSemantic(Video::Format format);

    ClearType getClearType(const String &clearType);

    uint32_t getTextureLoadFlags(const String &loadFlags);
    uint32_t getTextureFlags(const String &createFlags);
    uint32_t getBufferFlags(const String &createFlags);

    std::unordered_map<String, String> getAliasedMap(const JSON::Object &parent, const wchar_t *name);
}; // namespace Gek
