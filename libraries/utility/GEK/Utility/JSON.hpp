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
                return Gek::Evaluator::Get<float>(object.as_cstring());
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
                return Gek::Evaluator::Get<Gek::Math::Vector2<TYPE>>(object.as_cstring());
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
                return Gek::Evaluator::Get<Gek::Math::Vector3<TYPE>>(object.as_cstring());
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
                return Gek::Evaluator::Get<Gek::Math::Vector4<TYPE>>(object.as_cstring());
            }

            return Gek::Math::Vector4<TYPE>::Zero;
        }

        static Json to_json(const Gek::Math::Vector4<TYPE> &value)
        {
            return Gek::String::create(L"%v", value);
        }
    };

    template<class Json>
    struct json_type_traits<Json, Gek::Math::SIMD::Float4>
    {
        static Gek::Math::SIMD::Float4 as(const Json &object)
        {
            if (object.is_string())
            {
                return Gek::Evaluator::Get<Gek::Math::SIMD::Float4>(object.as_cstring());
            }

            return Gek::Math::SIMD::Float4::Zero;
        }

        static Json to_json(const Gek::Math::SIMD::Float4 &value)
        {
            return Gek::String::create(L"%v", value);
        }
    };

    template<class Json>
    struct json_type_traits<Json, Gek::Math::Quaternion>
    {
        static Gek::Math::Quaternion as(const Json &object)
        {
            if (object.is_string())
            {
                return Gek::Evaluator::Get<Gek::Math::Quaternion>(object.as_cstring());
            }

            return Gek::Math::Quaternion::Identity;
        }

        static Json to_json(const Gek::Math::Quaternion &value)
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

        Object Load(const wchar_t *fileName);
        void Save(const wchar_t *fileName, const Object &object);
	}; // namespace JSON
}; // namespace Gek
