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

template <class... Fs>
struct overload;

template <class F0, class... Frest>
struct overload<F0, Frest...> : F0, overload<Frest...>
{
    overload(F0 f0, Frest... rest) : F0(f0), overload<Frest...>(rest...) {}

    using F0::operator();
    using overload<Frest...>::operator();
};

template <class F0>
struct overload<F0> : F0
{
    overload(F0 f0) : F0(f0) {}

    using F0::operator();
};

__inline bool value_or_default(bool const &value, bool defaultValue)
{
    return value;
}

template <typename SOURCE_TYPE>
bool value_or_default(SOURCE_TYPE const &value, bool defaultValue)
{
    return defaultValue;
}

template <typename SOURCE_TYPE, typename TARGET_TYPE>
typename std::enable_if<std::is_convertible<SOURCE_TYPE, TARGET_TYPE>::value, TARGET_TYPE>::type
value_or_default(SOURCE_TYPE const &value, TARGET_TYPE defaultValue)
{
    return value;
}

template <typename SOURCE_TYPE, typename TARGET_TYPE>
typename std::enable_if<!std::is_convertible<SOURCE_TYPE, TARGET_TYPE>::value, TARGET_TYPE>::type
value_or_default(SOURCE_TYPE const &value, TARGET_TYPE defaultValue)
{
    return defaultValue;
}

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

        template <class... Fs>
        auto visit(Fs... fs)
        {
            return std::visit(overload<Fs...>(fs...), data);
        }

        template <class... Fs>
        auto visit(Fs... fs) const
        {
            return std::visit(overload<Fs...>(fs...), data);
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

        template <typename TYPE>
        TYPE &get(void)
        {
            if (!is<TYPE>())
            {
                data = TYPE();
            }

            return std::get<TYPE>(data);
        }

        JSON const &get(size_t index) const;
        JSON const &get(std::string_view name) const;

        JSON &operator [] (size_t index);
        JSON &operator [] (std::string_view name);

        template <typename TARGET_TYPE>
        constexpr TARGET_TYPE convert(TARGET_TYPE defaultValue) const
        {
            return visit(
                [&](auto const &visitedData) -> TARGET_TYPE
            {
                return value_or_default(visitedData, defaultValue);
            });
        }

        template <typename TYPE>
        TYPE evaluate(ShuntingYard &shuntingYard, TYPE defaultValue) const
        {
            return visit(
                [&](std::string const &visitedData) -> TYPE
            {
                return shuntingYard.evaluate(visitedData).value_or(defaultValue);
            },
                [&](Object const &visitedData) -> TYPE
            {
                return defaultValue;
            },
                [&](Array const &visitedData) -> TYPE
            {
                return defaultValue;
            },
                [&](auto && visitedData) -> TYPE
            {
                return visitedData;
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