/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include <jsoncons/json.hpp>

namespace jsoncons
{
    template<class Json>
    struct json_type_traits<Json, float>
    {
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
        static Gek::Math::Vector2<TYPE> as(const Json &object)
        {
            return Gek::String(object.as_cstring());
        }

        static Json to_json(const Gek::Math::Vector2<TYPE> &value)
        {
            return Gek::String::create(L"%v", value);
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::Vector3<TYPE>>
    {
        static Gek::Math::Vector3<TYPE> as(const Json &object)
        {
            return Gek::String(object.as_cstring());
        }

        static Json to_json(const Gek::Math::Vector3<TYPE> &value)
        {
            return Gek::String::create(L"%v", value);
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::Vector4<TYPE>>
    {
        static Gek::Math::Vector4<TYPE> as(const Json &object)
        {
            return Gek::String(object.as_cstring());
        }

        static Json to_json(const Gek::Math::Vector4<TYPE> &value)
        {
            return Gek::String::create(L"%v", value);
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::SIMD::Vector4<TYPE>>
    {
        static Gek::Math::SIMD::Vector4<TYPE> as(const Json &object)
        {
            return Gek::String(object.as_cstring());
        }

        static Json to_json(const Gek::Math::SIMD::Vector4<TYPE> &value)
        {
            return Gek::String::create(L"%v", value);
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::Quaternion<TYPE>>
    {
        static Gek::Math::Quaternion<TYPE> as(const Json &object)
        {
            return Gek::String(object.as_cstring());
        }

        static Json to_json(const Gek::Math::Quaternion<TYPE> &value)
        {
            return Gek::String::create(L"%v", value);
        }
    };
};

namespace Gek
{
    namespace JSON
    {
        using Object = jsoncons::wjson;
        using Member = Object::member_type;

        Object load(const wchar_t *fileName);
        void save(const wchar_t *fileName, const Object &object);
	}; // namespace JSON
}; // namespace Gek
