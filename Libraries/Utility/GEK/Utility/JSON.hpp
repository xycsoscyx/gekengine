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
#include <type_traits>
#include <functional>
#include <variant>

namespace Gek
{
    __inline bool GetValueOrDefault(std::string const &value, bool defaultValue)
    {
        std::istringstream stream(String::GetLower(value));
        bool result = defaultValue;
        stream >> std::boolalpha >> result;
        return result;
    }

    template <typename TARGET_TYPE>
    TARGET_TYPE GetValueOrDefault(std::nullptr_t const &, TARGET_TYPE defaultValue)
    {
        return defaultValue;
    }

    template <typename SOURCE_TYPE, typename TARGET_TYPE>
    typename std::enable_if<std::is_convertible<SOURCE_TYPE, TARGET_TYPE>::value, TARGET_TYPE>::type
        GetValueOrDefault(SOURCE_TYPE const &value, TARGET_TYPE defaultValue)
    {
        return value;
    }

    template <typename SOURCE_TYPE, typename TARGET_TYPE>
    typename std::enable_if<!std::is_convertible<SOURCE_TYPE, TARGET_TYPE>::value, TARGET_TYPE>::type
        GetValueOrDefault(SOURCE_TYPE const &value, TARGET_TYPE defaultValue)
    {
        return defaultValue;
    }

    template <class... FUNCTIONS>
    struct Overload
    {
    };

    template <class FIRST_FUNCTION, class... ADDITIONAL_FUNCTIONS>
    struct Overload<FIRST_FUNCTION, ADDITIONAL_FUNCTIONS...> : FIRST_FUNCTION, Overload<ADDITIONAL_FUNCTIONS...>
    {
        Overload(FIRST_FUNCTION firstFunction, ADDITIONAL_FUNCTIONS... additionalFunctions)
            : FIRST_FUNCTION(firstFunction)
            , Overload<ADDITIONAL_FUNCTIONS...>(additionalFunctions...)
        {
        }

        using FIRST_FUNCTION::operator();
        using Overload<ADDITIONAL_FUNCTIONS...>::operator();
    };

    template <class FUNCTION>
    struct Overload<FUNCTION>
        : FUNCTION
    {
        Overload(FUNCTION function)
            : FUNCTION(function)
        {
        }

        using FUNCTION::operator();
    };

    class JSON
    {
    public:
        using Array = std::vector<JSON>;
        using Object = std::unordered_map<std::string, JSON>;

        static const Array EmptyArray;
        static const Object EmptyObject;
        static const JSON Empty;

    private:
        std::variant<std::nullptr_t, bool, int32_t, uint32_t, int64_t, uint64_t, float, std::string, Array, Object> data;

    public:
        JSON(void)
        {
        }

        JSON(JSON const &node)
            : data(node.data)
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

        template <class... FUNCTIONS>
        auto visit(FUNCTIONS&&... functions)
        {
            return std::visit(Overload<FUNCTIONS...>(functions...), data);
        }

        template <class... FUNCTIONS>
        auto visit(FUNCTIONS&&... functions) const
        {
            return std::visit(Overload<FUNCTIONS...>(functions...), data);
        }

        template <typename TYPE>
        bool isType(void) const
        {
            return std::holds_alternative<TYPE>(data);
        }

        template <typename TYPE>
        TYPE asType(TYPE defaultValue) const
        {
            if (auto value = std::get_if<TYPE>(&data))
            {
                return *value;
            }

            return defaultValue;
        }

        template <typename TYPE>
        TYPE &makeType(void)
        {
            if (!isType<TYPE>())
            {
                data = TYPE();
            }

            return std::get<TYPE>(data);
        }

        JSON const &getIndex(size_t index) const;
        JSON &operator [] (size_t index);

        JSON const &getMember(std::string_view name) const;
        JSON &operator [] (std::string_view name);

        template <typename TARGET_TYPE>
        TARGET_TYPE convert(TARGET_TYPE defaultValue) const
        {
            return visit(
                [defaultValue](auto const &visitedData) -> TARGET_TYPE
            {
                return GetValueOrDefault(visitedData, defaultValue);
            });
        }

        template <typename TYPE>
        TYPE evaluate(ShuntingYard &shuntingYard, TYPE defaultValue) const
        {
            return visit(
                [&shuntingYard, defaultValue](std::string const &visitedData) -> TYPE
            {
                return shuntingYard.evaluate(visitedData).value_or(defaultValue);
            },
                [defaultValue](Object const &visitedData) -> TYPE
            {
                return defaultValue;
            },
                [defaultValue](Array const &visitedData) -> TYPE
            {
                return defaultValue;
            },
                [defaultValue](auto && visitedData) -> TYPE
            {
                return GetValueOrDefault(visitedData, defaultValue);
            });
        }

        Math::Float2 evaluate(ShuntingYard &shuntingYard, Math::Float2 const &defaultValue) const;
        Math::Float3 evaluate(ShuntingYard &shuntingYard, Math::Float3 const &defaultValue) const;
        Math::Float4 evaluate(ShuntingYard &shuntingYard, Math::Float4 const &defaultValue) const;
        Math::Quaternion evaluate(ShuntingYard &shuntingYard, Math::Quaternion const &defaultValue) const;
        std::string evaluate(ShuntingYard &shuntingYard, std::string const &defaultValue) const;
    };
}; // namespace Gek

namespace std
{
    inline std::string to_string(std::nullptr_t const &)
    {
        return "null"s;
    }

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