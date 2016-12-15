#include "GEK/Engine/Renderer.hpp"
#include "Passes.hpp"

namespace Gek
{
    ClearData::ClearData(ClearType type, const String &data)
        : type(type)
    {
        switch (type)
        {
        case ClearType::Float:
        case ClearType::Target:
            floats = data;
            break;
            
        case ClearType::UInt:
            integers[0] = integers[1] = integers[2] = integers[3] = data;
            break;
        };
    }

    String getFormatSemantic(Video::Format format)
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
            return L"float4";

        case Video::Format::R32G32B32_FLOAT:
        case Video::Format::R11G11B10_FLOAT:
            return L"float3";

        case Video::Format::R32G32_FLOAT:
        case Video::Format::R16G16_FLOAT:
        case Video::Format::R16G16_UNORM:
        case Video::Format::R8G8_UNORM:
        case Video::Format::R16G16_NORM:
        case Video::Format::R8G8_NORM:
            return L"float2";

        case Video::Format::R32_FLOAT:
        case Video::Format::R16_FLOAT:
        case Video::Format::R16_UNORM:
        case Video::Format::R8_UNORM:
        case Video::Format::R16_NORM:
        case Video::Format::R8_NORM:
        case Video::Format::D32_FLOAT_S8X24_UINT:
        case Video::Format::D24_UNORM_S8_UINT:
        case Video::Format::D32_FLOAT:
        case Video::Format::D16_UNORM:
            return L"float";

        case Video::Format::R32G32B32A32_UINT:
        case Video::Format::R16G16B16A16_UINT:
        case Video::Format::R10G10B10A2_UINT:
            return L"uint4";

        case Video::Format::R8G8B8A8_UINT:
        case Video::Format::R32G32B32_UINT:
            return L"uint3";

        case Video::Format::R32G32_UINT:
        case Video::Format::R16G16_UINT:
        case Video::Format::R8G8_UINT:
            return L"uint2";

        case Video::Format::R32_UINT:
        case Video::Format::R16_UINT:
        case Video::Format::R8_UINT:
            return L"uint";

        case Video::Format::R32G32B32A32_INT:
        case Video::Format::R16G16B16A16_INT:
        case Video::Format::R8G8B8A8_INT:
            return L"int4";

        case Video::Format::R32G32B32_INT:
            return L"int3";

        case Video::Format::R32G32_INT:
        case Video::Format::R16G16_INT:
        case Video::Format::R8G8_INT:
            return L"int2";

        case Video::Format::R32_INT:
        case Video::Format::R16_INT:
        case Video::Format::R8_INT:
            return L"int";
        };

        return L"";
    }

    ClearType getClearType(const String &clearType)
    {
        if (clearType.compareNoCase(L"Target") == 0) return ClearType::Target;
        else if (clearType.compareNoCase(L"Float") == 0) return ClearType::Float;
        else if (clearType.compareNoCase(L"UInt") == 0) return ClearType::UInt;
        return ClearType::Unknown;
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
                flags |= Video::Texture::Description::Flags::RenderTarget;
            }
            else if (flag.compareNoCase(L"depth") == 0)
            {
                flags |= Video::Texture::Description::Flags::DepthTarget;
            }
            else if (flag.compareNoCase(L"unorderedaccess") == 0)
            {
                flags |= Video::Texture::Description::Flags::UnorderedAccess;
            }
        }

        return (flags | Video::Texture::Description::Flags::Resource);
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
                flags |= Video::Buffer::Description::Flags::UnorderedAccess;
            }
            else if (flag.compareNoCase(L"counter") == 0)
            {
                flags |= Video::Buffer::Description::Flags::Counter;
            }
        }

        return (flags | Video::Buffer::Description::Flags::Resource);
    }

    std::unordered_map<String, String> getAliasedMap(const JSON::Object &parent, const wchar_t *name)
    {
        std::unordered_map<String, String> aliasedMap;
        if (parent.has_member(name))
        {
            auto &object = parent.get(name);
            if (object.is_array())
            {
                uint32_t mapCount = object.size();
                for (auto &element : object.elements())
                {
                    if (element.is_string())
                    {
                        String name(element.as_string());
                        aliasedMap[name] = name;
                    }
                    else if (element.is_object() && !element.empty())
                    {
                        auto &member = element.begin_members();
                        String name(member->name());
                        String value(member->value().as_string());
                        aliasedMap[name] = value;
                    }
                    else
                    {
                    }
                }
            }
            else
            {
            }
        }

        return aliasedMap;
    }
}; // namespace Gek