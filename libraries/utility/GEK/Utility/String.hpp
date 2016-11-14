/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 31a07b88fab4425367fa0aa67fe970fbff7dc9dc $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 27 08:51:53 2016 -0700 $
#pragma once

#include "GEK\Math\Vector2.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Vector4.hpp"
#include "GEK\Math\SIMD\Vector4.hpp"
#include "GEK\Math\Quaternion.hpp"
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
    template<class ELEMENT, class TRAITS = std::char_traits<ELEMENT>, class ALLOCATOR = std::allocator<ELEMENT>>
    class BaseString : public std::basic_string<ELEMENT, TRAITS, ALLOCATOR>
    {
    private:
        template <typename TYPE>
        struct traits;

        template <>
        struct traits<char>
        {
            static void convert(std::basic_string<wchar_t> &result, const char *input)
            {
                static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
                result.assign(convert.from_bytes(input));
            }
        };

        template <>
        struct traits<wchar_t>
        {
            static void convert(std::basic_string<char> &result, const wchar_t *input)
            {
                static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
    			result.assign(convert.to_bytes(input));
            }
        };

        template <typename DESTINATION, typename SOURCE>
        struct converter
        {
            static void convert(std::basic_string<DESTINATION> &result, const SOURCE *string)
            {
                traits<SOURCE>::convert(result, string);
            }
        };

        template <typename DESTINATION>
        struct converter<DESTINATION, DESTINATION>
        {
            static void convert(std::basic_string<DESTINATION> &result, const DESTINATION *string)
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

    public:
        using ElementType = ELEMENT;

        BaseString(void)
        {
        }

        BaseString(std::size_t length, ELEMENT character)
            : basic_string(length, character)
        {
        }

        BaseString(const ELEMENT *string)
        {
            if (string)
            {
                assign(string);
            }
        }

        template<typename CONVERSION>
        BaseString(const CONVERSION *string)
        {
            if (string)
            {
                converter<ELEMENT, CONVERSION>::convert((*this), string);
            }
        }

        template<typename CONVERSION>
        BaseString(const std::basic_string<CONVERSION> &string)
        {
            if (!string.empty())
            {
                converter<ELEMENT, CONVERSION>::convert((*this), string.data());
            }
        }

        template<typename CONVERSION>
        BaseString(const BaseString<CONVERSION> &string)
        {
            if (!string.empty())
            {
                converter<ELEMENT, CONVERSION>::convert((*this), string.data());
            }
        }

        BaseString subString(size_t position = 0, size_t length = std::string::npos) const
        {
            if (position >= size())
            {
                throw std::out_of_range("BaseString<ELEMENT>::subString() - position out of range");
            }

            return BaseString(substr(position, length));
        }

        bool replace(const BaseString &from, const BaseString &to)
        {
            bool replaced = false;

            size_t position = 0;
            while ((position = find(from, position)) != std::string::npos)
            {
                basic_string::replace(position, from.size(), to);
                position += to.length();
                replaced = true;
            };

            return replaced;
        }

        void trimLeft(const std::function<bool(ELEMENT)> &checkElement = [](ELEMENT ch) { return !std::isspace(ch, std::locale::classic()); })
        {
            erase(begin(), std::find_if(begin(), end(), checkElement));
        }

        void trimRight(const std::function<bool(ELEMENT)> &checkElement = [](ELEMENT ch) { return !std::isspace(ch, std::locale::classic()); })
        {
            erase(std::find_if(rbegin(), rend(), checkElement).base(), end());
        }

        void trim(const std::function<bool(ELEMENT)> &checkElement = [](ELEMENT ch) { return !std::isspace(ch, std::locale::classic()); })
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

        BaseString & join(const std::vector<BaseString> &list, ELEMENT delimiter)
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

        static BaseString create(const std::vector<BaseString> &list, ELEMENT delimiter)
        {
            BaseString result;
            result.join(list, delimiter);
            return result;
        }

		bool endsWith(const BaseString &string) const
		{
			if (string.length() > length()) return false;
			return std::equal(string.rbegin(), string.rend(), rbegin());
		}

		int compareNoCase(const BaseString &string) const
        {
            if (size() != string.size())
            {
                return 1;
            }

            return std::equal(std::begin(*this), std::end(*this), std::begin(string), [](const ELEMENT &left, const ELEMENT &right) -> bool
            {
                return (std::tolower(left) == std::tolower(right));
            }) ? 0 : 1;
        }

        BaseString & append(std::size_t length, ELEMENT character)
        {
            return static_cast<BaseString &>(std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(length, character));
        }

        BaseString & append(const BaseString &string)
        {
            return static_cast<BaseString &>(std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(string));
        }

        BaseString & format(const BaseString &string)
        {
            return static_cast<BaseString &>(std::basic_string<ELEMENT, TRAITS, ALLOCATOR>::append(string));
        }

        template<typename TYPE, typename... PARAMETERS>
        BaseString & format(const ELEMENT *formatting, const TYPE &value, PARAMETERS... arguments)
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
                        return format(formatting, arguments...);
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

        template<typename TYPE, typename... PARAMETERS>
        static BaseString create(const ELEMENT *formatting, const TYPE &value, PARAMETERS... arguments)
        {
            BaseString result;
            result.format(formatting, value, arguments...);
            return result;
        }

        BaseString &operator = (const ELEMENT &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (const bool &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::boolalpha << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (const float &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::showpoint << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (const long &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(8) << std::hex << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (const unsigned long &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(8) << std::hex << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        template <typename TYPE>
        BaseString &operator = (const Math::Vector2<TYPE> &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ')';
            return static_cast<BaseString &>(assign(stream.str()));
        }

        template <typename TYPE>
        BaseString &operator = (const Math::Vector3<TYPE> &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ')';
            return static_cast<BaseString &>(assign(stream.str()));
        }

        template <typename TYPE>
        BaseString &operator = (const Math::Vector4<TYPE> &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.r << ',' << value.g << ',' << value.b << ',' << value.a << ')';
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (const Math::SIMD::Float4 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (const Math::Quaternion &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            return static_cast<BaseString &>(assign(stream.str()));
        }

        BaseString &operator = (const ELEMENT *string)
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

        template <typename TYPE>
        BaseString &operator = (const TYPE &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<BaseString &>(assign(stream.str()));
        }

        template <typename CONVERSION>
        BaseString &operator = (const CONVERSION *string)
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

        template <typename CONVERSION>
        BaseString &operator = (const basic_string<CONVERSION> &string)
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

        template <typename CONVERSION>
        BaseString &operator = (const BaseString<CONVERSION> &string)
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

        void operator += (const ELEMENT &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const bool &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::boolalpha << value;
            append(stream.str());
        }

        void operator += (const float &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::showpoint << value;
            append(stream.str());
        }

        void operator += (const long &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(8) << std::hex << value;
            append(stream.str());
        }

        void operator += (const unsigned long &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(8) << std::hex << value;
            append(stream.str());
        }

        template <typename TYPE>
        void operator += (const Math::Vector2<TYPE> &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ')';
            append(stream.str());
        }

        template <typename TYPE>
        void operator += (const Math::Vector3<TYPE> &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ')';
            append(stream.str());
        }

        template <typename TYPE>
        void operator += (const Math::Vector4<TYPE> &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.r << ',' << value.g << ',' << value.b << ',' << value.a << ')';
            append(stream.str());
        }

        void operator += (const Math::SIMD::Float4 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            append(stream.str());
        }

        void operator += (const Math::Quaternion &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            append(stream.str());
        }

        void operator += (const ELEMENT *string)
        {
            if (string)
            {
                append(string);
            }
        }

        template <typename TYPE>
        void operator += (const TYPE &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        template <typename CONVERSION>
        void operator += (const CONVERSION *string)
        {
            append(BaseString(string));
        }

        template <typename CONVERSION>
        void operator += (const basic_string<CONVERSION> &string)
        {
            append(BaseString(string));
        }

        template <typename CONVERSION>
        void operator += (const BaseString<CONVERSION> &string)
        {
            append(BaseString(string));
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

        template <typename TYPE>
        operator Math::Vector2<TYPE> () const
        {
            Math::Vector2<TYPE> value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> MustMatch('(') >> value.x >> MustMatch(',') >> value.y >> MustMatch(')'); // ( X , Y )
            return (stream.fail() ? Math::Vector2<TYPE>::Zero : value);
        }

        template <typename TYPE>
        operator Math::Vector3<TYPE> () const
        {
            Math::Vector3<TYPE> value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> MustMatch('(') >> value.x >> MustMatch(',') >> value.y >> MustMatch(',') >> value.z >> MustMatch(')'); // ( x , Y , Z )
            return (stream.fail() ? Math::Vector3<TYPE>::Zero : value);
        }

		template <typename TYPE>
		operator Math::Vector4<TYPE>() const
		{
			Math::Vector4<TYPE> value;
			std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
			if (stream.peek() == '(')
			{
				stream >> MustMatch('(') >> value.r >> MustMatch(',') >> value.g >> MustMatch(',') >> value.b;
				switch (stream.peek())
				{
				case ')':
					value.a = TYPE(1);
					stream >> MustMatch(')');
					break;

				case ',':
					stream >> MustMatch(',') >> value.a >> MustMatch(')');
					break;
				};
			}
			else
			{
				stream >> value.r;
				value.set(value.r);
			}

			return (stream.fail() ? Math::Vector4<TYPE>::Zero : value);
		}

        operator Math::SIMD::Float4() const
        {
            Math::SIMD::Float4 value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> MustMatch('(') >> value.x >> MustMatch(',') >> value.y >> MustMatch(',') >> value.z >> MustMatch(',') >> value.w >> MustMatch(')'); // ( X , Y , Z , W )
            return (stream.fail() ? Math::SIMD::Float4::Zero : value);
        }

		operator Math::Quaternion () const
        {
            Math::Quaternion value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> MustMatch('(') >> value.x >> MustMatch(',') >> value.y >> MustMatch(',') >> value.z;
            switch (stream.peek())
            {
            case ')':
                value = Math::Quaternion::FromEuler(value.x, value.y, value.z);
                stream >> MustMatch(')');
                break;

            case ',':
                stream >> MustMatch(',') >> value.w >> MustMatch(')');
                break;
            };

            return (stream.fail() ? Math::Quaternion::Identity : value);
        }

        operator const ELEMENT * () const
        {
            return data();
        }
    };

    using StringUTF8 = BaseString<char>;
    using String = BaseString<wchar_t>;
}; // namespace Gek

namespace std
{
    template<>
    struct hash<Gek::StringUTF8>
    {
        size_t operator()(const Gek::StringUTF8 &value) const
        {
            return hash<string>()(value);
        }
    };

    template<>
    struct hash<Gek::String>
    {
        size_t operator()(const Gek::String &value) const
        {
            return hash<wstring>()(value);
        }
    };
}; // namespace std