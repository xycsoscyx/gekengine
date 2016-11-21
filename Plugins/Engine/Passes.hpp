/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 347000ea4cf5625a5cd325614cb63ebc84e060aa $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Sat Oct 15 19:36:10 2016 +0000 $
#pragma once

#include "GEK\Utility\String.hpp"
#include "GEK\Utility\JSON.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Engine\Resources.hpp"

namespace Gek
{
    enum class BindType : uint8_t
    {
        Unknown = 0,
        Bool,
        Int, Int2, Int3, Int4,
        UInt, UInt2, UInt3, UInt4,
        Half, Half2, Half3, Half4,
        Float, Float2, Float3, Float4,
        Structure,
    };

    enum class MapType : uint8_t
    {
        Unknown = 0,
        Texture1D,
        Texture2D,
        Texture2DMS,
        TextureCube,
        Texture3D,
        Buffer,
        ByteAddressBuffer,
    };

    enum class MapSource : uint8_t
    {
        Unknown = 0,
        File,
        Pattern,
        Resource,
    };

    struct Map
    {
        MapType type;
        MapSource source;
        BindType binding;
        uint32_t flags;
        union
        {
            String fileName;
            String resourceName;
            String pattern;
        };

        String parameters;

        Map(MapType type, BindType binding, uint32_t flags, const String &fileName);
        Map(MapType type, BindType binding, uint32_t flags, const String &pattern, const String &parameters);
        Map(const String &resourceName);
        Map(const Map &map);
        ~Map(void);

        Map & operator = (const Map &map);
    };

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
        union
        {
            Math::Float4 color;
            Math::Float4 value;
            uint32_t uint[4];
        };

        ClearData(ClearType type, const String &data);
        ClearData(const ClearData &clearData);
        ~ClearData(void);

        ClearData & operator = (const ClearData &clearData);
    };

    BindType getBindType(const String &bindType);
    String getBindType(BindType bindType);
    const BindType getBindType(Video::Format format);
    const Video::Format getBindFormat(BindType bindType);

    MapType getMapType(const String &mapType);
    String getMapType(MapType mapType);

    ClearType getClearType(const String &clearType);

    uint32_t getTextureLoadFlags(const String &loadFlags);
    uint32_t getTextureFlags(const String &createFlags);
    uint32_t getBufferFlags(const String &createFlags);

    std::unordered_map<String, String> getAliasedMap(const JSON::Object &parent, const wchar_t *name);

    Video::Format getElementFormat(const String &format);
    Video::InputElement::Source getElementSource(const String &elementSource);
    Video::InputElement::Semantic getElementSemantic(const String &semantic);
}; // namespace Gek
