/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector2.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ShuntingYard.hpp"
#include <variant>

namespace Gek
{
    class JSON
    {
    public:
        using Array = std::vector<JSON>;
        using Object = std::unordered_map<std::string, JSON>;

        static const Array EmptyArray;
        static const Object EmptyObject;
        static const JSON Empty;

    private:
        std::variant<bool, int32_t, uint32_t, int64_t, uint64_t, float, std::string, Array, Object> data;

    public:
        JSON(void)
        {
        }

        template <typename TYPE>
        JSON(TYPE newData)
            : data(newData)
        {
        }

        void load(FileSystem::Path const &filePath);
        void save(FileSystem::Path const &filePath);

        std::string getString(void) const;

        template <typename FUNCTION>
        auto visit(FUNCTION function) const
        {
            return std::visit(function, data);
        }

        bool empty(void) const
        {
            return (data.index() == std::variant_npos);
        }

        template <typename TYPE>
        bool is(void) const
        {
            return std::holds_alternative<TYPE>(data);
        }

        template <typename TYPE>
        TYPE as(TYPE defaultValue) const
        {
            if (auto value = std::get_if<TYPE>(&data))
            {
                return *value;
            }

            return defaultValue;
        }

        JSON const &get(size_t index) const;
        JSON const &get(std::string_view name) const;

        JSON &operator [] (size_t index);
        JSON &operator [] (std::string_view name);

        int32_t evaluate(ShuntingYard &shuntingYard, int32_t defaultValue) const;
        uint32_t evaluate(ShuntingYard &shuntingYard, uint32_t defaultValue) const;
        int64_t evaluate(ShuntingYard &shuntingYard, int64_t defaultValue) const;
        uint64_t evaluate(ShuntingYard &shuntingYard, uint64_t defaultValue) const;
        float evaluate(ShuntingYard &shuntingYard, float defaultValue) const;
        Math::Float2 evaluate(ShuntingYard &shuntingYard, Math::Float2 const &defaultValue) const;
        Math::Float3 evaluate(ShuntingYard &shuntingYard, Math::Float3 const &defaultValue) const;
        Math::Float4 evaluate(ShuntingYard &shuntingYard, Math::Float4 const &defaultValue) const;
        Math::Quaternion evaluate(ShuntingYard &shuntingYard, Math::Quaternion const &defaultValue) const;
        std::string evaluate(ShuntingYard &shuntingYard, std::string const &defaultValue) const;
    };
}; // namespace Gek

namespace std
{
    inline std::string to_string(Gek::JSON const &data)
    {
        return data.getString();
    }

    inline std::string to_string(Gek::JSON::Array const &data)
    {
        return Gek::JSON(data).getString();
    }

    inline std::string to_string(Gek::JSON::Object const &data)
    {
        return Gek::JSON(data).getString();
    }
};