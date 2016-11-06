#include "GEK\Engine\Renderer.hpp"
#include "Passes.hpp"

namespace Gek
{
    Map::Map(MapType type, BindType binding, uint32_t flags, const wchar_t *fileName)
        : source(MapSource::File)
        , type(type)
        , binding(binding)
        , flags(flags)
        , fileName(fileName)
    {
    }

    Map::Map(MapType type, BindType binding, uint32_t flags, const wchar_t *pattern, const wchar_t *parameters)
        : source(MapSource::Pattern)
        , type(type)
        , binding(binding)
        , flags(flags)
        , pattern(pattern)
        , parameters(parameters)
    {
    }

    Map::Map(const wchar_t *resourceName)
        : source(MapSource::Resource)
        , resourceName(resourceName)
    {
    }

    Map::Map(const Map &map)
        : type(map.type)
        , source(map.source)
        , binding(map.binding)
        , flags(map.flags)
        , fileName(map.fileName)
        , parameters(map.parameters)
    {
    }

    Map::~Map(void)
    {
    }

    Map & Map::operator = (const Map &map)
    {
        type = map.type;
        source = map.source;
        binding = map.binding;
        flags = map.flags;
        fileName = map.fileName;
        parameters = map.parameters;
        return *this;
    }

    ClearData::ClearData(ClearType type, const String &data)
        : type(type)
    {
        switch (type)
        {
        case ClearType::Float:
            color = data;
            break;

        case ClearType::Target:
            value = data;
            break;
            
        case ClearType::UInt:
            uint[0] = uint[1] = uint[2] = uint[3] = data;
            break;
        };
    }

    ClearData::ClearData(const ClearData &clearData)
        : type(clearData.type)
    {
        switch (type)
        {
        case ClearType::Target:
            color = clearData.color;
            break;

        case ClearType::Float:
            value = clearData.value;
            break;

        case ClearType::UInt:
            uint[0] = clearData.uint[0];
            uint[1] = clearData.uint[1];
            uint[2] = clearData.uint[2];
            uint[3] = clearData.uint[3];
            break;
        };
    }

    ClearData::~ClearData(void)
    {
    }

    ClearData & ClearData::operator = (const ClearData &clearData)
    {
        type = clearData.type;
        switch (type)
        {
        case ClearType::Target:
            color = clearData.color;
            break;

        case ClearType::Float:
            value = clearData.value;
            break;

        case ClearType::UInt:
            uint[0] = clearData.uint[0];
            uint[1] = clearData.uint[1];
            uint[2] = clearData.uint[2];
            uint[3] = clearData.uint[3];
            break;
        };

        return *this;
    }

    Video::Format getElementSource(const wchar_t *type)
    {
        if (wcsicmp(type, L"float") == 0) return Video::Format::R32_FLOAT;
        else if (wcsicmp(type, L"float2") == 0) return Video::Format::R32G32_FLOAT;
        else if (wcsicmp(type, L"float3") == 0) return Video::Format::R32G32B32_FLOAT;
        else if (wcsicmp(type, L"float4") == 0) return Video::Format::R32G32B32A32_FLOAT;
        else if (wcsicmp(type, L"int") == 0) return Video::Format::R32_INT;
        else if (wcsicmp(type, L"int2") == 0) return Video::Format::R32G32_INT;
        else if (wcsicmp(type, L"int3") == 0) return Video::Format::R32G32B32_INT;
        else if (wcsicmp(type, L"int4") == 0) return Video::Format::R32G32B32A32_INT;
        else if (wcsicmp(type, L"uint") == 0) return Video::Format::R32_UINT;
        else if (wcsicmp(type, L"uint2") == 0) return Video::Format::R32G32_UINT;
        else if (wcsicmp(type, L"uint3") == 0) return Video::Format::R32G32B32_UINT;
        else if (wcsicmp(type, L"uint4") == 0) return Video::Format::R32G32B32A32_UINT;
        return Video::Format::Unknown;
    }

    ClearType getClearType(const String &clearType)
    {
        if (clearType.compareNoCase(L"Target") == 0) return ClearType::Target;
        else if (clearType.compareNoCase(L"Float") == 0) return ClearType::Float;
        else if (clearType.compareNoCase(L"UInt") == 0) return ClearType::UInt;
        return ClearType::Unknown;
    }

    MapType getMapType(const String &mapType)
    {
        if (mapType.compareNoCase(L"Texture1D") == 0) return MapType::Texture1D;
        else if (mapType.compareNoCase(L"Texture2D") == 0) return MapType::Texture2D;
        else if (mapType.compareNoCase(L"Texture3D") == 0) return MapType::Texture3D;
        else if (mapType.compareNoCase(L"Buffer") == 0) return MapType::Buffer;
        else if (mapType.compareNoCase(L"ByteAddressBuffer") == 0) return MapType::ByteAddressBuffer;
        return MapType::Unknown;
    }

    String getMapType(MapType mapType)
    {
        switch (mapType)
        {
        case MapType::Texture1D:            return L"Texture1D";
        case MapType::Texture2D:            return L"Texture2D";
        case MapType::TextureCube:          return L"TextureCube";
        case MapType::Texture3D:            return L"Texture3D";
        case MapType::Buffer:               return L"Buffer";
        case MapType::ByteAddressBuffer:    return L"ByteAddressBuffer";
        };

        return L"void";
    }

    BindType getBindType(const String &bindType)
    {
        if (bindType.compareNoCase(L"Float") == 0) return BindType::Float;
        else if (bindType.compareNoCase(L"Float2") == 0) return BindType::Float2;
        else if (bindType.compareNoCase(L"Float3") == 0) return BindType::Float3;
        else if (bindType.compareNoCase(L"Float4") == 0) return BindType::Float4;

        else if (bindType.compareNoCase(L"Half") == 0) return BindType::Half;
        else if (bindType.compareNoCase(L"Half2") == 0) return BindType::Half2;
        else if (bindType.compareNoCase(L"Half3") == 0) return BindType::Half3;
        else if (bindType.compareNoCase(L"Half4") == 0) return BindType::Half4;

        else if (bindType.compareNoCase(L"Int") == 0) return BindType::Int;
        else if (bindType.compareNoCase(L"Int2") == 0) return BindType::Int2;
        else if (bindType.compareNoCase(L"Int3") == 0) return BindType::Int3;
        else if (bindType.compareNoCase(L"Int4") == 0) return BindType::Int4;

        else if (bindType.compareNoCase(L"UInt") == 0) return BindType::UInt;
        else if (bindType.compareNoCase(L"UInt2") == 0) return BindType::UInt2;
        else if (bindType.compareNoCase(L"UInt3") == 0) return BindType::UInt3;
        else if (bindType.compareNoCase(L"UInt4") == 0) return BindType::UInt4;

        else if (bindType.compareNoCase(L"Bool") == 0) return BindType::Bool;

        return BindType::Unknown;
    }

    String getBindType(BindType bindType)
    {
        switch (bindType)
        {
        case BindType::Float:       return L"float";
        case BindType::Float2:      return L"float2";
        case BindType::Float3:      return L"float3";
        case BindType::Float4:      return L"float4";

        case BindType::Half:        return L"half";
        case BindType::Half2:       return L"half2";
        case BindType::Half3:       return L"half3";
        case BindType::Half4:       return L"half4";

        case BindType::Int:         return L"int";
        case BindType::Int2:        return L"int2";
        case BindType::Int3:        return L"int3";
        case BindType::Int4:        return L"int4";

        case BindType::UInt:        return L"uint";
        case BindType::UInt2:       return L"uint2";
        case BindType::UInt3:       return L"uint3";
        case BindType::UInt4:       return L"uint4";

        case BindType::Bool:        return L"bool";
        };

        return L"void";
    }

    const BindType getBindType(Video::Format format)
    {
        switch (format)
        {
        case Video::Format::R32G32B32A32_FLOAT:
        case Video::Format::R16G16B16A16_FLOAT:
        case Video::Format::R16G16B16A16_UNORM:
        case Video::Format::R10G10B10A2_UNORM:
        case Video::Format::R8G8B8A8_UNORM:
        case Video::Format::R8G8B8A8_UNORM_SRGB:
        case Video::Format::R16G16B16A16_NORM:
        case Video::Format::R8G8B8A8_NORM:
            return BindType::Float4;

        case Video::Format::R32G32B32_FLOAT:
        case Video::Format::R11G11B10_FLOAT:
            return BindType::Float3;

        case Video::Format::R32G32_FLOAT:
        case Video::Format::R16G16_FLOAT:
        case Video::Format::R16G16_UNORM:
        case Video::Format::R8G8_UNORM:
        case Video::Format::R16G16_NORM:
        case Video::Format::R8G8_NORM:
            return BindType::Float2;

        case Video::Format::R32_FLOAT:
        case Video::Format::R16_FLOAT:
        case Video::Format::R16_UNORM:
        case Video::Format::R8_UNORM:
        case Video::Format::R16_NORM:
        case Video::Format::R8_NORM:
            return BindType::Float;

        case Video::Format::R32G32B32A32_UINT:
        case Video::Format::R16G16B16A16_UINT:
        case Video::Format::R10G10B10A2_UINT:
        case Video::Format::R8G8B8A8_UINT:
            return BindType::UInt4;

        case Video::Format::R32G32B32_UINT:
        case Video::Format::R32G32B32_INT:
            return BindType::UInt3;

        case Video::Format::R32G32_UINT:
        case Video::Format::R16G16_UINT:
        case Video::Format::R8G8_UINT:
            return BindType::UInt2;

        case Video::Format::R32_UINT:
        case Video::Format::R16_UINT:
        case Video::Format::R8_UINT:
            return BindType::UInt;

        case Video::Format::R32G32B32A32_INT:
        case Video::Format::R16G16B16A16_INT:
        case Video::Format::R8G8B8A8_INT:
            return BindType::Int4;

        case Video::Format::R32G32_INT:
        case Video::Format::R16G16_INT:
        case Video::Format::R8G8_INT:
            return BindType::Int2;

        case Video::Format::R32_INT:
        case Video::Format::R16_INT:
        case Video::Format::R8_INT:
            return BindType::Int;
        };

        return BindType::Unknown;
    }

    const Video::Format getBindFormat(BindType bindType)
    {
        switch (bindType)
        {
        case BindType::Float4:
            return Video::Format::R32G32B32A32_FLOAT;

        case BindType::Float3:
            return Video::Format::R32G32B32_FLOAT;

        case BindType::Float2:
            return Video::Format::R32G32_FLOAT;

        case BindType::Float:
            return Video::Format::R32_FLOAT;

        case BindType::UInt4:
            return Video::Format::R32G32B32A32_UINT;

        case BindType::UInt3:
            return Video::Format::R32G32B32_UINT;

        case BindType::UInt2:
            return Video::Format::R32G32_UINT;

        case BindType::UInt:
            return Video::Format::R32_UINT;

        case BindType::Int4:
            return Video::Format::R32G32B32A32_INT;

        case BindType::Int2:
            return Video::Format::R32G32_INT;

        case BindType::Int:
            return Video::Format::R32_INT;
        };

        return Video::Format::Unknown;
    }

    uint32_t getTextureLoadFlags(const String &loadFlags)
    {
        uint32_t flags = 0;
        int position = 0;
        std::vector<String> flagList(loadFlags.split(L','));
        for (auto &flag : flagList)
        {
            if (flag.compareNoCase(L"sRGB") == 0)
            {
                flags |= Video::TextureLoadFlags::sRGB;
            }
        }

        return flags;
    }

    uint32_t getTextureFlags(const String &createFlags)
    {
        uint32_t flags = 0;
        int position = 0;
        std::vector<String> flagList(createFlags.split(L','));
        for (auto &flag : flagList)
        {
            flag.trim();
            if (flag.compareNoCase(L"target") == 0)
            {
                flags |= Video::TextureFlags::RenderTarget;
            }
            else if (flag.compareNoCase(L"depth") == 0)
            {
                flags |= Video::TextureFlags::DepthTarget;
            }
            else if (flag.compareNoCase(L"unorderedaccess") == 0)
            {
                flags |= Video::TextureFlags::UnorderedAccess;
            }
        }

        return (flags | Video::TextureFlags::Resource);
    }

    uint32_t getBufferFlags(const String &createFlags)
    {
        uint32_t flags = 0;
        int position = 0;
        std::vector<String> flagList(createFlags.split(L','));
        for (auto &flag : flagList)
        {
            flag.trim();
            if (flag.compareNoCase(L"unorderedaccess") == 0)
            {
                flags |= Video::BufferFlags::UnorderedAccess;
            }
            else if (flag.compareNoCase(L"counter") == 0)
            {
                flags |= Video::BufferFlags::Counter;
            }
        }

        return (flags | Video::BufferFlags::Resource);
    }

    std::unordered_map<String, String> getAliasedMap(const JSON::Object &object)
    {
        std::unordered_map<String, String> aliasedMap;
        if (object.is_array())
        {
            uint32_t mapCount = object.size();
            for (auto &element : object.elements())
            {
                if (element.is_string())
                {
                    String name(element.as_cstring());
                    aliasedMap[name] = name;
                }
                else if(element.is_object() && !element.empty())
                {
                    auto member = element.begin_members();
                    String name(member->name());
                    String value(member->value().as_cstring());
                    aliasedMap[name] = value;
                }
                else
                {
                }
            }
        }

        return aliasedMap;
    }
}; // namespace Gek