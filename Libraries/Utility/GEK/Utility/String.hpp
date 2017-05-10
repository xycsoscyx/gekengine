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
	static const std::string EmptyString;

	template <typename CONTAINER>
	void TrimLeft(CONTAINER &string, std::function<bool(typename CONTAINER::value_type)> checkElement = [](typename CONTAINER::value_type ch) { return !std::isspace(ch, std::locale::classic()); })
	{
		string.erase(std::begin(string), std::find_if(std::begin(string), std::end(string), checkElement));
	}

	template <typename CONTAINER>
	void TrimRight(CONTAINER &string, std::function<bool(typename CONTAINER::value_type)> checkElement = [](typename CONTAINER::value_type ch) { return !std::isspace(ch, std::locale::classic()); })
	{
		string.erase(std::find_if(std::rbegin(string), std::rend(string), checkElement).base(), std::end(string));
	}

	template <typename CONTAINER>
	void Trim(CONTAINER &string, std::function<bool(typename CONTAINER::value_type)> checkElement = [](typename CONTAINER::value_type ch) { return !std::isspace(ch, std::locale::classic()); })
	{
		TrimLeft(string, checkElement);
		TrimRight(string, checkElement);
	}

	template <typename CONTAINER>
	CONTAINER GetLower(CONTAINER &string)
	{
		CONTAINER transformed;
		std::transform(std::begin(string), std::end(string), std::back_inserter(transformed), ::tolower);
		return transformed;
	}

	template <typename CONTAINER>
	CONTAINER GetUpper(CONTAINER &string)
	{
		CONTAINER transformed;
		std::transform(std::begin(string), std::end(string), std::back_inserter(transformed), ::toupper);
		return transformed;
	}

	static std::string Join(std::vector<std::string> const &list, char delimiter, bool initialDelimiter = false)
    {
		if (list.empty())
		{
			return EmptyString;
		}

		auto size = (initialDelimiter ? 1 : 0); // initial length
		size += (list.size() - 1); // insert delimiters between list elements
		for (const auto &string : list)
		{
			size += string.length();
		}

		std::string result;
		result.reserve(size);
		if (initialDelimiter)
		{
			result.append(1U, delimiter);
		}

		result.append(list.front());
		for (auto &string = std::next(std::begin(list), 1); string != std::end(list); ++string)
		{
			result.append(1U, delimiter);
			result.append(*string);
		}

		return result;
    }

	std::vector<std::string> Split(const std::string &string, char delimiter, bool clearSpaces = true)
	{
		std::string current;
		std::vector<std::string> tokens;
		std::stringstream stream(string);
		while (std::getline(stream, current, delimiter))
		{
			if (clearSpaces)
			{
				Trim(current);
			}

			tokens.push_back(current);
		};

		return tokens;
	}

	char const *Format(char const *string)
	{
		return string;
	}

	template<typename TYPE, typename... PARAMETERS>
	std::string Format(char const *formatting, TYPE const &value, PARAMETERS... arguments)
	{
		std::string result;
		while (formatting && *formatting)
		{
			char currentCharacter = *formatting++;
			if (currentCharacter == char('%') && formatting && *formatting)
			{
				char nextCharacter = *formatting++;
				if (nextCharacter == char('%'))
				{
					// %%
					result.append(1U, nextCharacter);
				}
				else if (nextCharacter == char('v'))
				{
					// %v
					// use += operator for automatic type conversion
					// will fail at compile time with unknown types
					return (result + Format(formatting, arguments...));
				}
				else
				{
					// %(other)
					result.append(1U, currentCharacter).append(1U, nextCharacter);
				}
			}
			else
			{
				result.append(1U, currentCharacter);
			}
		};

		return result;
	}

	bool Replace(std::string &string, std::string const &searchFor, std::string const &replaceWith)
	{
		bool did_replace = false;

		size_t position = 0;
		while ((position = string.find(searchFor, position)) != std::string::npos)
		{
			string.replace(position, searchFor.size(), replaceWith);
			position += replaceWith.length();
			did_replace = true;
		};

		return did_replace;
	}

	void try_this(void)
	{
		auto string = Format("%v%v", 1, 2);
	}

/*
        std::string &operator = (char const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << value;
            return static_cast<std::string &>(assign(stream.str()));
        }

        std::string &operator = (bool const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << std::boolalpha << value;
            return static_cast<std::string &>(assign(stream.str()));
        }

        std::string &operator = (float const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << std::showpoint << value;
            return static_cast<std::string &>(assign(stream.str()));
        }

        std::string &operator = (long const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << std::uppercase << std::setfill(char('0')) << std::setw(8) << std::hex << value;
            return static_cast<std::string &>(assign(stream.str()));
        }

        std::string &operator = (unsigned long const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << std::uppercase << std::setfill(char('0')) << std::setw(8) << std::hex << value;
            return static_cast<std::string &>(assign(stream.str()));
        }

        template <typename TYPE>
        std::string &operator = (TYPE const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << value;
            return static_cast<std::string &>(assign(stream.str()));
        }

        std::string &operator = (char const *string)
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
        std::string &operator = (CONTAINER<char, std::char_traits<char>, std::allocator<char>> const &string)
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
        std::string &operator = (CONVERSION const *string)
        {
            if (string)
            {
                converter<char, CONVERSION>::convert((*this), string);
            }
            else
            {
                clear();
            }

            return (*this);
        }

        template<template <typename, typename, typename> class CONTAINER, typename CONVERSION>
        std::string &operator = (CONTAINER<CONVERSION, std::char_traits<CONVERSION>, std::allocator<CONVERSION>> const &string)
        {
            if (string.empty())
            {
                clear();
            }
            else
            {
                converter<char, CONVERSION>::convert((*this), string.data());
            }

            return (*this);
        }

        void operator += (char const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << value;
            std::basic_string<char, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (bool const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << std::boolalpha << value;
            std::basic_string<char, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (float const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << std::showpoint << value;
            std::basic_string<char, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (long const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << std::uppercase << std::setfill(char('0')) << std::setw(8) << std::hex << value;
            std::basic_string<char, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (unsigned long const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << std::uppercase << std::setfill(char('0')) << std::setw(8) << std::hex << value;
            std::basic_string<char, TRAITS, ALLOCATOR>::append(stream.str());
        }

        template <typename TYPE>
        void operator += (TYPE const &value)
        {
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream;
            stream << value;
            std::basic_string<char, TRAITS, ALLOCATOR>::append(stream.str());
        }

        void operator += (char const *string)
        {
            if (string)
            {
                std::basic_string<char, TRAITS, ALLOCATOR>::append(string);
            }
        }

        template<template <typename, typename, typename> class CONTAINER>
        void operator += (CONTAINER<char, std::char_traits<char>, std::allocator<char>> const &string)
        {
            std::basic_string<char, TRAITS, ALLOCATOR>::append(std::begin(string), std::end(string));
        }

        template <typename CONVERSION>
        void operator += (const CONVERSION *string)
        {
            std::string converted(string);
            std::basic_string<char, TRAITS, ALLOCATOR>::append(std::begin(converted), std::end(converted));
        }

        template<template <typename, typename, typename> class CONTAINER, typename CONVERSION>
        void operator += (CONTAINER<CONVERSION, std::char_traits<CONVERSION>, std::allocator<CONVERSION>> const &string)
        {
            std::string converted(string);
            std::basic_string<char, TRAITS, ALLOCATOR>::append(std::begin(converted), std::end(converted));
        }

        operator bool() const
        {
            bool value;
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream(*this);
            stream >> std::boolalpha >> value;
            return (stream.fail() ? false : value);
        }

        operator float () const
        {
            float value;
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0.0f : value);
        }

        operator int32_t () const
        {
            int32_t value;
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0 : value);
        }

        operator uint32_t () const
        {
            uint32_t value;
            std::basic_stringstream<char, std::char_traits<char>, std::allocator<char>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0 : value);
        }

        operator char const * const () const
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
            char character;
            MustMatch(char character)
                : character(character)
            {
            }

            template <typename STREAM>
            friend STREAM &operator >> (STREAM &stream, const MustMatch &match)
            {
                char next;
                stream.get(next);
                if (next != match.character)
                {
                    stream.setstate(std::ios::failbit);
                }

                return stream;
            }
        };
    };
	*/
}; // namespace Gek