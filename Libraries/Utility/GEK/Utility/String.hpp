/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 31a07b88fab4425367fa0aa67fe970fbff7dc9dc $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 27 08:51:53 2016 -0700 $
#pragma once

#include "GEK/Math/Vector2.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include <functional>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <codecvt>
#include <vector>
#include <cctype>
#include <string>

namespace Gek
{
	template <typename TYPE>
	struct EmptyString
	{
	};

	template<>
	struct EmptyString<char>
	{
		static char const * Get(void)
		{
			return "";
		}
	};

	template<>
	struct EmptyString<wchar_t>
	{
		static wchar_t const * Get(void)
		{
			return L"";
		}
	};

    template<class ELEMENT, class TRAITS = std::char_traits<ELEMENT>, class ALLOCATOR = std::allocator<ELEMENT>>
    class BaseString : public std::basic_string<ELEMENT, TRAITS, ALLOCATOR>
    {
    public:
        using ElementType = ELEMENT;
		static const BaseString<ELEMENT, TRAITS, ALLOCATOR> Empty;

    public:
        static BaseString Join(std::vector<BaseString> const &list, ELEMENT delimiter)
        {
            BaseString result;
            result.join(list, delimiter);
            return result;
        }

        template<typename TYPE, typename... PARAMETERS>
        static BaseString Format(ELEMENT const *formatting, TYPE const &value, PARAMETERS... arguments)
        {
            BaseString result;
            result.format(formatting, value, arguments...);
            return result;
        }

    public:
        BaseString(void)
        {
        }

        BaseString(std::size_t length, ELEMENT character)
            : basic_string(length, character)
        {
        }

		BaseString(ELEMENT const *string)
			: basic_string(string ? string : EmptyString<ELEMENT>::Get())
		{
		}

        template<template <typename, typename, typename> class CONTAINER>
		BaseString(CONTAINER<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> const &string)
			: basic_string(string.empty() ? Empty : string)
		{
		}

        template<typename CONVERSION>
        BaseString(CONVERSION const *string)
        {
			converter<ELEMENT, CONVERSION>::convert((*this), (string ? string : EmptyString<CONVERSION>::Get()));
        }

        template<template <typename, typename, typename> class CONTAINER, typename CONVERSION>
        BaseString(CONTAINER<CONVERSION, std::char_traits<CONVERSION>, std::allocator<CONVERSION>> const &string)
        {
			converter<ELEMENT, CONVERSION>::convert((*this), string.data());
        }

		BaseString subString(size_t position = 0, size_t length = BaseString::npos) const
        {
            if (position >= size())
            {
                throw std::out_of_range("BaseString<ELEMENT>::subString() - position out of range");
            }

            if ((position + length) >= size())
            {
                length = std::string::npos;
            }

            return BaseString(substr(position, length));
        }

        bool replace(BaseString const &searchFor, BaseString const &replaceWith)
        {
            bool replaced = false;

            size_t position = 0;
            while ((position = find(searchFor, position)) != std::string::npos)
            {
                basic_string::replace(position, searchFor.size(), replaceWith);
                position += replaceWith.length();
                replaced = true;
            };

            return replaced;
        }

        void trimLeft(std::function<bool(ELEMENT)> checkElement = [](ELEMENT ch) { return !std::isspace(ch, std::locale::classic()); })
        {
            erase(begin(), std::find_if(begin(), end(), checkElement));
        }

        void trimRight(std::function<bool(ELEMENT)> checkElement = [](ELEMENT ch) { return !std::isspace(ch, std::locale::classic()); })
        {
            erase(std::find_if(rbegin(), rend(), checkElement).base(), end());
        }

        void trim(std::function<bool(ELEMENT)> checkElement = [](ELEMENT ch) { return !std::isspace(ch, std::locale::classic()); })
        {
            trimLeft(checkElement);
            trimRight(checkElement);
        }

        void toLower(void)
        {
			std::transform(begin(), end(), begin(), ::tolower);
        }

        void toUpper(void)
        {
            std::transform(begin(), end(), begin(), ::toupper);
        }

        BaseString getLower(void) const
        {
            BaseString transformed;
            std::transform(begin(), end(), std::back_inserter(transformed), ::tolower);
            return transformed;
        }

        BaseString getUpper(void) const
        {
            BaseString transformed;
            std::transform(begin(), end(), std::back_inserter(transformed), ::toupper);
            return transformed;
        }

        std::vector<BaseString> split(ELEMENT delimiter, bool clearSpaces = true) const
        {
            BaseString current;
            std::vector<BaseString> tokens;
            std::basic_stringstream<ELEMENT, TRAITS, ALLOCATOR> stream(data());
            while (std::getline(stream, current, delimiter))
            {
                if (clearSpaces)
                {
                    current.trim();
                }

                tokens.push_back(current);
            };

            return tokens;
        }

        BaseString & join(std::vector<BaseString> const &list, ELEMENT delimiter)
        {
            if (list.empty())
            {
                throw std::out_of_range("BaseString<ELEMENT>::join() - empty list passed as parameter");
            }

            bool initialDelimiter = (!empty() && back() != delimiter);
            auto size = (length() + (initialDelimiter ? 1 : 0)); // initial length
            size += (list.size() - 1); // insert delimiters between list elements
            for (auto &stringSearch = std::begin(list); stringSearch != std::end(list); ++stringSearch)
            {
                size += (*stringSearch).length();
            }

            reserve(size);
            if (initialDelimiter)
            {
                append(1U, delimiter);
            }

            append(list.front());
            for (auto &stringSearch = std::next(std::begin(list), 1); stringSearch != std::end(list); ++stringSearch)
            {
                append(1U, delimiter);
                append(*stringSearch);
            }

            return (*this);
        }

		bool endsWith(BaseString const &string) const
		{
			if (string.length() > length()) return false;
			return std::equal(string.rbegin(), string.rend(), rbegin());
		}

		int compareNoCase(BaseString const &string) const
        {
            if (size() != string.size())
            {
                return 1;
            }

            return std::equal(std::begin(*this), std::end(*this), std::begin(string), [](ELEMENT const &left, ELEMENT const &right) -> bool
            {
                return (std::tolower(left) == std::tolower(right));
            }) ? 0 : 1;
        }

        BaseString & append(std::size_t length, ELEMENT character)
        {
            return static_cast<BaseString &>(std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(length, character));
        }

        BaseString & append(BaseString const &string)
        {
            return static_cast<BaseString &>(std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(string));
        }

        BaseString & appendFormat(ELEMENT const *string)
        {
            return static_cast<BaseString &>(std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(string));
        }

        template<typename TYPE, typename... PARAMETERS>
        BaseString & appendFormat(ELEMENT const *formatting, TYPE const &value, PARAMETERS... arguments)
        {
            while (formatting && *formatting)
            {
                ELEMENT currentCharacter = *formatting++;
                if (currentCharacter == ELEMENT('%') && formatting && *formatting)
                {
                    ELEMENT nextCharacter = *formatting++;
                    if (nextCharacter == ELEMENT('%'))
                    {
                        // %%
                        append(1U, nextCharacter);
                    }
                    else if (nextCharacter == ELEMENT('v'))
                    {
                        // %v
                        // use += operator for automatic type conversion
                        // will fail at compile time with unknown types
                        (*this) += value;
                        return appendFormat(formatting, arguments...);
                    }
                    else
                    {
                        // %(other)
                        append(1U, currentCharacter).append(1U, nextCharacter);
                    }
                }
                else
                {
                    append(1U, currentCharacter);
                }
            };

            return (*this);
        }

        template<typename... PARAMETERS>
        BaseString & format(ELEMENT const *formatting, PARAMETERS... arguments)
        {
            clear();
            return appendFormat(formatting, arguments...);
        }

        BaseString &operator = (ELEMENT const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (bool const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::boolalpha << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (float const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::showpoint << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (long const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(8) << std::hex << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (unsigned long const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(8) << std::hex << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        template <typename TYPE>
        BaseString &operator = (TYPE const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (ELEMENT const *string)
        {
            if (string)
            {
                assign(string);
            }
            else
            {
                clear();
            }

            return (*this);
        }

        template<template <typename, typename, typename> class CONTAINER>
        BaseString &operator = (CONTAINER<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> const &string)
        {
            if (string.empty())
            {
                clear();
            }
            else
            {
                assign(string);
            }

            return (*this);
        }

        template <typename CONVERSION>
        BaseString &operator = (CONVERSION const *string)
        {
            if (string)
            {
                converter<ELEMENT, CONVERSION>::convert((*this), string);
            }
            else
            {
                clear();
            }

            return (*this);
        }

        template<template <typename, typename, typename> class CONTAINER, typename CONVERSION>
        BaseString &operator = (CONTAINER<CONVERSION, std::char_traits<CONVERSION>, std::allocator<CONVERSION>> const &string)
        {
            if (string.empty())
            {
                clear();
            }
            else
            {
                converter<ELEMENT, CONVERSION>::convert((*this), string.data());
            }

            return (*this);
        }

        void operator += (ELEMENT const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (bool const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::boolalpha << value;
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (float const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::showpoint << value;
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (long const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(8) << std::hex << value;
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (unsigned long const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(8) << std::hex << value;
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(stream.str());
        }

        template <typename TYPE>
        void operator += (TYPE const &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (ELEMENT const *string)
        {
            if (string)
            {
                std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(string);
            }
        }

        template<template <typename, typename, typename> class CONTAINER>
        void operator += (CONTAINER<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> const &string)
        {
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(std::begin(string), std::end(string));
        }

        template <typename CONVERSION>
        void operator += (const CONVERSION *string)
        {
            BaseString converted(string);
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(std::begin(converted), std::end(converted));
        }

        template<template <typename, typename, typename> class CONTAINER, typename CONVERSION>
        void operator += (CONTAINER<CONVERSION, std::char_traits<CONVERSION>, std::allocator<CONVERSION>> const &string)
        {
            BaseString converted(string);
            std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(std::begin(converted), std::end(converted));
        }

        operator bool() const
        {
            bool value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> std::boolalpha >> value;
            return (stream.fail() ? false : value);
        }

        operator float () const
        {
            float value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0.0f : value);
        }

        operator int32_t () const
        {
            int32_t value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0 : value);
        }

        operator uint32_t () const
        {
            uint32_t value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0 : value);
        }

        operator ELEMENT const * const () const
        {
            return data();
        }

    private:
        template <typename TYPE>
        struct traits;

        template <>
        struct traits<char>
        {
            static void convert(std::basic_string<wchar_t> &result, char const * const input)
            {
                static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
                result.assign(convert.from_bytes(input));
            }
        };

        template <>
        struct traits<wchar_t>
        {
            static void convert(std::basic_string<char> &result, wchar_t const * const input)
            {
                static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
                result.assign(convert.to_bytes(input));
            }
        };

        template <typename TARGET_TYPE, typename SOURCE_TYPE>
        struct converter
        {
            static void convert(std::basic_string<TARGET_TYPE> &result, SOURCE_TYPE const *string)
            {
                traits<SOURCE_TYPE>::convert(result, string);
            }
        };

        template <typename TARGET_TYPE>
        struct converter<TARGET_TYPE, TARGET_TYPE>
        {
            static void convert(std::basic_string<TARGET_TYPE> &result, TARGET_TYPE const *string)
            {
                result.assign(string);
            }
        };

        struct MustMatch
        {
            ELEMENT character;
            MustMatch(ELEMENT character)
                : character(character)
            {
            }

            template <typename STREAM>
            friend STREAM &operator >> (STREAM &stream, const MustMatch &match)
            {
                ELEMENT next;
                stream.get(next);
                if (next != match.character)
                {
                    stream.setstate(std::ios::failbit);
                }

                return stream;
            }
        };
    };

    using CString = BaseString<char>;
    using WString = BaseString<wchar_t>;
}; // namespace Gek

namespace std
{
    template<>
    struct hash<Gek::CString>
    {
        size_t operator()(Gek::CString const &value) const
        {
            return hash<string>()(value);
        }
    };

    template<>
    struct hash<Gek::WString>
    {
        size_t operator()(Gek::WString const &value) const
        {
            return hash<wstring>()(value);
        }
    };
}; // namespace std