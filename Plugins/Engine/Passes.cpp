#include "GEK/Engine/Renderer.hpp"
#include "Passes.hpp"

namespace Gek
{
    ClearData::ClearData(ClearType type, std::string const &data)
        : type(type)
    {
        switch (type)
        {
        case ClearType::Float:
        case ClearType::Target:
			floats.set(String::Convert(data, 0.0f));
            break;
            
        case ClearType::UInt:
			integers[0] = integers[1] = integers[2] = integers[3] = String::Convert(data, 0U);
            break;
        };
    }

    std::string getFormatSemantic(Video::Format format)
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
            return "float4";

        case Video::Format::R32G32B32_FLOAT:
        case Video::Format::R11G11B10_FLOAT:
            return "float3";

        case Video::Format::R32G32_FLOAT:
        case Video::Format::R16G16_FLOAT:
        case Video::Format::R16G16_UNORM:
        case Video::Format::R8G8_UNORM:
        case Video::Format::R16G16_NORM:
        case Video::Format::R8G8_NORM:
            return "float2";

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
            return "float";

        case Video::Format::R32G32B32A32_UINT:
        case Video::Format::R16G16B16A16_UINT:
        case Video::Format::R10G10B10A2_UINT:
            return "uint4";

        case Video::Format::R8G8B8A8_UINT:
        case Video::Format::R32G32B32_UINT:
            return "uint3";

        case Video::Format::R32G32_UINT:
        case Video::Format::R16G16_UINT:
        case Video::Format::R8G8_UINT:
            return "uint2";

        case Video::Format::R32_UINT:
        case Video::Format::R16_UINT:
        case Video::Format::R8_UINT:
            return "uint";

        case Video::Format::R32G32B32A32_INT:
        case Video::Format::R16G16B16A16_INT:
        case Video::Format::R8G8B8A8_INT:
            return "int4";

        case Video::Format::R32G32B32_INT:
            return "int3";

        case Video::Format::R32G32_INT:
        case Video::Format::R16G16_INT:
        case Video::Format::R8G8_INT:
            return "int2";

        case Video::Format::R32_INT:
        case Video::Format::R16_INT:
        case Video::Format::R8_INT:
            return "int";
        };

		return String::Empty;
    }

    std::string getFormatSemantic(Video::Format format, uint32_t count)
    {
        std::string semantic(getFormatSemantic(format));
        if (count > 0)
        {
            semantic += String::Format("x%v", count);
        }

        return semantic;
    }

    ClearType getClearType(std::string const &string)
    {
		static const std::unordered_map<std::string, ClearType> data =
		{
			{ "target", ClearType::Target },
			{ "float", ClearType::Float },
			{ "uint", ClearType::UInt },
		};

		auto result = data.find(String::GetLower(string));
		return (result == std::end(data) ? ClearType::Unknown : result->second);
    }

    uint32_t getTextureLoadFlags(std::string const &loadFlags)
    {
        uint32_t flags = 0;
        int position = 0;
		std::vector<std::string> flagList(String::Split(String::GetLower(loadFlags), ','));
        for (const auto &flag : flagList)
        {
            if (flag == "srgb")
            {
                flags |= Video::TextureLoadFlags::sRGB;
            }
        }

        return flags;
    }

    uint32_t getTextureFlags(std::string const &createFlags)
    {
        uint32_t flags = 0;
        int position = 0;
        std::vector<std::string> flagList(String::Split(String::GetLower(createFlags), ','));
        for (const auto &flag : flagList)
        {
            if (flag == "target")
            {
                flags |= Video::Texture::Description::Flags::RenderTarget;
            }
            else if (flag == "depth")
            {
                flags |= Video::Texture::Description::Flags::DepthTarget;
            }
            else if (flag == "unorderedaccess")
            {
                flags |= Video::Texture::Description::Flags::UnorderedAccess;
            }
        }

        return (flags | Video::Texture::Description::Flags::Resource);
    }

    uint32_t getBufferFlags(std::string const &createFlags)
    {
        uint32_t flags = 0;
        int position = 0;
        std::vector<std::string> flagList(String::Split(String::GetLower(createFlags), ','));
        for (const auto &flag : flagList)
        {
            if (flag == "unorderedaccess")
            {
                flags |= Video::Buffer::Description::Flags::UnorderedAccess;
            }
            else if (flag == "counter")
            {
                flags |= Video::Buffer::Description::Flags::Counter;
            }
        }

        return (flags | Video::Buffer::Description::Flags::Resource);
    }

    std::unordered_map<std::string, std::string> getAliasedMap(JSON::Reference parent, std::string const &name)
    {
        std::unordered_map<std::string, std::string> aliasedMap;
        auto &object = parent.get(name);
        for (auto &element : object.getArray())
        {
            if (element.is_string())
            {
                std::string name(element.as_string());
                aliasedMap[name] = name;
            }
            else if (element.is_object() && !element.empty())
            {
                auto &member = element.begin_members();
                std::string name(member->name());
                std::string value(member->value().as_string());
                aliasedMap[name] = value;
            }
        }

        return aliasedMap;
    }
}; // namespace Gek