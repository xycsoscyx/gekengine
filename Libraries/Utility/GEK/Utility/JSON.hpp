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
        std::variant<bool, int64_t, uint64_t, float, std::string, Array, Object> data;

    public:
        void load(FileSystem::Path const &filePath);
        void save(FileSystem::Path const &filePath);

        template <typename FUNCTION>
        void visit(FUNCTION function) const
        {
            std::visit(function, data);
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
        TYPE & operator = (TYPE value)
        {
            return std::get<TYPE>(data = value);
        }

        JSON const &get(size_t index) const;
        JSON const &get(std::string_view name) const;

        JSON &operator [] (size_t index);
        JSON &operator [] (std::string_view name);
    };
}; // namespace Gek
