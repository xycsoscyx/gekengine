/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include <jsoncons/json.hpp>

namespace jsoncons
{
    template<class Json>
    struct json_type_traits<Json, float>
    {
        static bool is(Json const &object)
        {
            return object.is_double();
        }

        static float as(Json const &object)
        {
            return float(object.as_double());
        }

        static Json to_json(float const &value)
        {
            return double(value);
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::Vector2<TYPE>>
    {
        static Json to_json(Gek::Math::Vector2<TYPE> const &value)
        {
            return Json::array {
                value.x,
                value.y
            };
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::Vector3<TYPE>>
    {
        static Json to_json(Gek::Math::Vector3<TYPE> const &value)
        {
            return Json::array {
                value.x,
                value.y,
                value.z
            };
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::Vector4<TYPE>>
    {
        static Json to_json(Gek::Math::Vector4<TYPE> const &value)
        {
            return Json::array {
                value.x,
                value.y,
                value.z,
                value.w
            };
        }
    };

    template<class Json>
    struct json_type_traits<Json, Gek::Math::Quaternion>
    {
        static Json to_json(Gek::Math::Quaternion const &value)
        {
            return Json::array {
                value.x,
                value.y,
                value.z,
                value.w
            };
        }
    };
};

namespace Gek
{
    namespace JSON
    {
        using Object = jsoncons::wjson;
        using Member = Object::member_type;
        using Array = Object::array;

		extern const Object EmptyObject;

        Object Load(FileSystem::Path const &filePath, const Object &defaultValue = EmptyObject);
        void Save(FileSystem::Path const &filePath, Object const &object);
	}; // namespace JSON
}; // namespace Gek
