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
        static bool is(const Json &object)
        {
            return object.is_double();
        }

        static float as(const Json &object)
        {
            return float(object.as_double());
        }

        static Json to_json(const float &value)
        {
            return double(value);
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::Vector2<TYPE>>
    {
        static Json to_json(const Gek::Math::Vector2<TYPE> &value)
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
        static Json to_json(const Gek::Math::Vector3<TYPE> &value)
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
        static Json to_json(const Gek::Math::Vector4<TYPE> &value)
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
        static Json to_json(const Gek::Math::Quaternion &value)
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

        Object Load(const FileSystem::Path &filePath);
        void Save(const FileSystem::Path &filePath, const Object &object);
	}; // namespace JSON
}; // namespace Gek
