/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Utility\String.hpp"
#include "GEK\Utility\Evaluator.hpp"
#include <jsoncons/json.hpp>

namespace jsoncons
{
    template<class Json>
    struct json_type_traits<Json, float>
    {
        static float as(const Json &object)
        {
            if (object.is_string())
            {
                return Gek::Evaluator::get<float>(object.as_cstring());
            }
            else
            {
                return float(object.as_double());
            }
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
            if (object.is_string())
            {
                return Gek::Evaluator::get<Gek::Math::Vector2<TYPE>>(object.as_cstring());
            }

            return Gek::Math::Vector2<TYPE>::Zero;
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
            if (object.is_string())
            {
                return Gek::Evaluator::get<Gek::Math::Vector3<TYPE>>(object.as_cstring());
            }

            return Gek::Math::Vector3<TYPE>::Zero;
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
            if (object.is_string())
            {
                return Gek::Evaluator::get<Gek::Math::Vector4<TYPE>>(object.as_cstring());
            }

            return Gek::Math::Vector4<TYPE>::Zero;
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
            if (object.is_string())
            {
                return Gek::Evaluator::get<Gek::Math::SIMD::Vector4<TYPE>>(object.as_cstring());
            }

            return Gek::Math::SIMD::Vector4<TYPE>::Zero;
        }

        static Json to_json(const Gek::Math::SIMD::Vector4<TYPE> &value)
        {
            return Gek::String::create(L"%v", value);
        }
    };

    template<typename TYPE, class Json>
    struct json_type_traits<Json, Gek::Math::BaseQuaternion<TYPE>>
    {
        static Gek::Math::BaseQuaternion<TYPE> as(const Json &object)
        {
            if (object.is_string())
            {
                return Gek::Evaluator::get<Gek::Math::BaseQuaternion<TYPE>>(object.as_cstring());
            }

            return Gek::Math::BaseQuaternion<TYPE>::Identity;
        }

        static Json to_json(const Gek::Math::BaseQuaternion<TYPE> &value)
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
