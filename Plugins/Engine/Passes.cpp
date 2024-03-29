#include "GEK/API/Visualizer.hpp"
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
            integers.set(String::Convert(data, 0U));
            break;
        };
    }

    std::string getFormatSemantic(Render::Format format)
    {
        switch (format)
        {
        case Render::Format::R32G32B32A32_FLOAT:
        case Render::Format::R16G16B16A16_FLOAT:
        case Render::Format::R16G16B16A16_UNORM:
        case Render::Format::R10G10B10A2_UNORM:
        case Render::Format::R8G8B8A8_UNORM:
        case Render::Format::R8G8B8A8_UNORM_SRGB:
        case Render::Format::R16G16B16A16_NORM:
        case Render::Format::R8G8B8A8_NORM:
            return "float4"s;

        case Render::Format::R32G32B32_FLOAT:
        case Render::Format::R11G11B10_FLOAT:
            return "float3"s;

        case Render::Format::R32G32_FLOAT:
        case Render::Format::R16G16_FLOAT:
        case Render::Format::R16G16_UNORM:
        case Render::Format::R8G8_UNORM:
        case Render::Format::R16G16_NORM:
        case Render::Format::R8G8_NORM:
            return "float2"s;

        case Render::Format::R32_FLOAT:
        case Render::Format::R16_FLOAT:
        case Render::Format::R16_UNORM:
        case Render::Format::R8_UNORM:
        case Render::Format::R16_NORM:
        case Render::Format::R8_NORM:
        case Render::Format::D32_FLOAT_S8X24_UINT:
        case Render::Format::D24_UNORM_S8_UINT:
        case Render::Format::D32_FLOAT:
        case Render::Format::D16_UNORM:
            return "float"s;

        case Render::Format::R32G32B32A32_UINT:
        case Render::Format::R16G16B16A16_UINT:
        case Render::Format::R10G10B10A2_UINT:
            return "uint4"s;

        case Render::Format::R8G8B8A8_UINT:
        case Render::Format::R32G32B32_UINT:
            return "uint3"s;

        case Render::Format::R32G32_UINT:
        case Render::Format::R16G16_UINT:
        case Render::Format::R8G8_UINT:
            return "uint2"s;

        case Render::Format::R32_UINT:
        case Render::Format::R16_UINT:
        case Render::Format::R8_UINT:
            return "uint"s;

        case Render::Format::R32G32B32A32_INT:
        case Render::Format::R16G16B16A16_INT:
        case Render::Format::R8G8B8A8_INT:
            return "int4"s;

        case Render::Format::R32G32B32_INT:
            return "int3"s;

        case Render::Format::R32G32_INT:
        case Render::Format::R16G16_INT:
        case Render::Format::R8G8_INT:
            return "int2"s;

        case Render::Format::R32_INT:
        case Render::Format::R16_INT:
        case Render::Format::R8_INT:
            return "int"s;
        };

		return String::Empty;
    }

    std::string getFormatSemantic(Render::Format format, uint32_t count)
    {
        std::string semantic(getFormatSemantic(format));
        if (count > 1)
        {
            semantic += std::format("x{}", count);
        }

        return semantic;
    }

    ClearType getClearType(std::string const &string)
    {
		static const std::unordered_map<std::string, ClearType> data =
		{
			{ "target"s, ClearType::Target },
			{ "float"s, ClearType::Float },
			{ "uint"s, ClearType::UInt },
		};

		auto result = data.find(String::GetLower(string));
		return (result == std::end(data) ? ClearType::Unknown : result->second);
    }

    uint32_t getTextureLoadFlags(std::string const &loadFlags)
    {
        uint32_t flags = 0;
        int position = 0;
		std::vector<std::string> flagList(String::Split(String::GetLower(loadFlags), ','));
        for (auto const &flag : flagList)
        {
            if (flag == "srgb"s)
            {
                flags |= Render::TextureLoadFlags::sRGB;
            }
        }

        return flags;
    }

    uint32_t getTextureFlags(std::string const &createFlags)
    {
        uint32_t flags = 0;
        int position = 0;
        std::vector<std::string> flagList(String::Split(String::GetLower(createFlags), ','));
        for (auto const &flag : flagList)
        {
            if (flag == "target"s)
            {
                flags |= Render::Texture::Flags::RenderTarget;
            }
            else if (flag == "depth"s)
            {
                flags |= Render::Texture::Flags::DepthTarget;
            }
            else if (flag == "unorderedaccess"s)
            {
                flags |= Render::Texture::Flags::UnorderedAccess;
            }
        }

        return (flags | Render::Texture::Flags::Resource);
    }

    uint32_t getBufferFlags(std::string const &createFlags)
    {
        uint32_t flags = 0;
        int position = 0;
        std::vector<std::string> flagList(String::Split(String::GetLower(createFlags), ','));
        for (auto const &flag : flagList)
        {
            if (flag == "unorderedaccess"s)
            {
                flags |= Render::Buffer::Flags::UnorderedAccess;
            }
            else if (flag == "counter"s)
            {
                flags |= Render::Buffer::Flags::Counter;
            }
        }

        return (flags | Render::Buffer::Flags::Resource);
    }

    std::unordered_map<std::string, std::string> getAliasedMap(JSON::Object const &parent, std::string_view group)
    {
        auto &node = JSON::Find(parent, group);
        std::unordered_map<std::string, std::string> aliasedMap;
        for (auto &elementNode : node)
        {
            if (elementNode.is_string())
            {
                auto element = elementNode.get<std::string>();
                aliasedMap[element] = element;
            }
            else if (elementNode.is_object())
            {
                auto firstMember = elementNode.begin();
                auto &aliasName = firstMember.key();
                auto &aliasNode = firstMember.value();
                if (aliasNode.is_string())
                {
                    aliasedMap[aliasName] = aliasNode.get<std::string>();
                }
            }
        }

        return aliasedMap;
    }
}; // namespace Gek