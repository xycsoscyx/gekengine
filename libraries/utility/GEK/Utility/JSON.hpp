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

namespace Gek
{
    namespace JSON
    {
        using Object = jsoncons::wjson;
        using Member = Object::member_type;

        Object load(const wchar_t *fileName);

		template <typename ELEMENT>
		BaseString<ELEMENT> getValue(const Object &object, const ELEMENT *defaultValue)
		{
			return (object.is_null() ? defaultValue : object.as_cstring());
		}

		template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
		TYPE getValue(const Object &object, TYPE defaultValue)
		{
			return (object.is_null() ? defaultValue : object.as<TYPE>());
		}

		template <typename TYPE, typename = typename std::enable_if<!std::is_arithmetic<TYPE>::value, TYPE>::type>
		TYPE getValue(const Object &object, const TYPE &defaultValue)
		{
			return (object.is_null() ? defaultValue : String(object.as_cstring()));
		}

		template <typename ELEMENT>
		BaseString<ELEMENT> getMember(const Object &object, const wchar_t *name, const ELEMENT *defaultValue)
		{
			auto &member = object[name];
			return (member.is_null() ? defaultValue : member.as_cstring());
		}

		template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
		TYPE getMember(const Object &object, const wchar_t *name, TYPE defaultValue)
		{
			auto &member = object[name];
			return (member.is_null() ? defaultValue : member.as<TYPE>());
		}

		template <typename TYPE, typename = typename std::enable_if<!std::is_arithmetic<TYPE>::value, TYPE>::type>
		TYPE getMember(const Object &object, const wchar_t *name, const TYPE &defaultValue)
		{
			auto &member = object[name];
			return (member.is_null() ? defaultValue : String(member.as_cstring()));
		}

		template <typename ELEMENT>
		void setValue(Object &object, const ELEMENT *value)
		{
			object = value;
		}

		template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
		void setValue(Object &object, TYPE value)
		{
			object = value;
		}

		template <typename TYPE, typename = typename std::enable_if<!std::is_arithmetic<TYPE>::value, TYPE>::type>
		void setValue(Object &object, const TYPE &value)
		{
			object = String::create(L"%v", value);
		}

		template <typename ELEMENT>
		void setMember(Object &object, const wchar_t *name, const ELEMENT *value)
		{
			object.set(name, value);
		}

		template <typename TYPE, typename = typename std::enable_if<std::is_arithmetic<TYPE>::value, TYPE>::type>
		void setMember(Object &object, const wchar_t *name, TYPE value)
		{
			object.set(name, value);
		}

		template <typename TYPE, typename = typename std::enable_if<!std::is_arithmetic<TYPE>::value, TYPE>::type>
		void setMember(Object &object, const wchar_t *name, const TYPE &value)
		{
			object.set(name, String::create(L"%v", value));
		}
	}; // namespace JSON
}; // namespace Gek
