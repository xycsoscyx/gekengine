/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Utility\String.hpp"
#include <jsoncons/json.hpp>

namespace Gek
{
    namespace JSON
    {
        using Object = jsoncons::wjson;

        template <typename ELEMENT>
        BaseString<ELEMENT> getValue(const Object &object, const ELEMENT *defaultValue)
        {
            return (object.is_null() ? defaultValue : object.as<TYPE>());
        }

        template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
        TYPE getValue(const Object &object, TYPE defaultValue)
        {
            return (object.is_null() ? defaultValue : object.as<TYPE>());
        }

        template <typename TYPE>
        TYPE getValue(const Object &object, const TYPE &defaultValue)
        {
            return (object.is_null() ? defaultValue : String(object.as_cstring()));
        }

        template <typename TYPE>
        TYPE getMember(const Object &object, const wchar_t *name, const TYPE &defaultValue)
        {
            auto &value = object[name];
            return (value.is_null() ? defaultValue : value.as<TYPE>());
        }
    }; // namespace JSON
}; // namespace Gek
