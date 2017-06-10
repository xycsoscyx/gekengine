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

        ClearData(ClearType type, std::string const &data);
    };

    std::string getFormatSemantic(Video::Format format);
    std::string getFormatSemantic(Video::Format format, uint32_t count);

    ClearType getClearType(std::string const &clearType);

    uint32_t getTextureLoadFlags(std::string const &loadFlags);
    uint32_t getTextureFlags(std::string const &createFlags);
    uint32_t getBufferFlags(std::string const &createFlags);

    std::unordered_map<std::string, std::string> getAliasedMap(JSON::Reference parent, std::string const &name);
}; // namespace Gek
