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
        ResourceHandle resource;

        Map(MapSource source, MapType type, BindType binding, ResourceHandle resource);
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
        Math::Float4 floats;
        Math::UInt4 integers;

        ClearData(ClearType type, const String &data);
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
