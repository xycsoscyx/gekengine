#pragma once

#include "GEK\Math\Float2.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Utility\Trace.h"
#include <Windows.h>
#include <functional>
#include <sstream>
#include <codecvt>
#include <vector>
#include <cctype>

namespace Gek
{
    template<class ELEMENT, class TRAITS = std::char_traits<ELEMENT>, class ALLOCATOR = std::allocator<ELEMENT>>
    class baseString : public std::basic_string<ELEMENT, TRAITS, ALLOCATOR>
    {
    private:
        template <typename TYPE>
        struct traits;

        template <>
        struct traits<char>
        {
            static void convert(std::basic_string<wchar_t> &result, const char *input)
            {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
                result.assign(convert.from_bytes(input));
            }
        };

        template <>
        struct traits<wchar_t>
        {
            static void convert(std::basic_string<char> &result, const wchar_t *input)
            {
                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
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

    public:
        baseString(void)
        {
        }

        baseString(ELEMENT character, size_t length = 1)
            : basic_string(length, character)
        {
        }

        template<typename TYPE, typename... ARGUMENTS>
        baseString(const ELEMENT *formatting, const TYPE &value, ARGUMENTS... arguments)
        {
            if (formatting)
            {
                format(formatting, value, arguments...);
            }
        }

        template<typename CONVERSION>
        baseString(const CONVERSION *string)
        {
            if (string)
            {
                converter<ELEMENT, CONVERSION>::convert(*this, string);
            }
        }

        template<typename CONVERSION>
        baseString(const basic_string<CONVERSION> &string)
        {
            if (!string.empty())
            {
                converter<ELEMENT, CONVERSION>::convert(*this, string.c_str());
            }
        }

        template<typename CONVERSION>
        baseString(const baseString<CONVERSION> &string)
        {
            if (!string.empty())
            {
                converter<ELEMENT, CONVERSION>::convert(*this, string.c_str());
            }
        }

        baseString subString(size_t position = 0, size_t length = std::string::npos) const
        {
            return baseString(substr(position, length));
        }

        bool replace(const baseString &from, const baseString &to)
        {
            bool replaced = false;
            size_t position = 0;
            while ((position = find(from, position)) != std::string::npos)
            {
                basic_string::replace(position, from.size(), to);
                position += to.length();
                replaced = true;
            };

            return true;
        }

        void trimLeft(void)
        {
            erase(begin(), std::find_if(begin(), end(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); }));
        }

        void trimRight(void)
        {
            erase(std::find_if(rbegin(), rend(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); }).base(), end());
        }

        void trim(void)
        {
            trimLeft();
            trimRight();
        }

        void toLower(void)
        {
            std::transform(begin(), end(), begin(), ::tolower);
        }

        void toUpper(void)
        {
            std::transform(begin(), end(), begin(), ::toupper);
        }

        baseString<ELEMENT> getLower(void) const
        {
            baseString transformed(ELEMENT(' '), length());
            std::transform(begin(), end(), transformed.begin(), ::tolower);
            return transformed;
        }

        baseString<ELEMENT> getUpper(void) const
        {
            baseString transformed(ELEMENT(' '), length());
            std::transform(begin(), end(), transformed.begin(), ::toupper);
            return transformed;
        }

        std::vector<baseString<ELEMENT>> split(ELEMENT delimiter) const
        {
            baseString current;
            std::vector<baseString> tokens;
            std::basic_stringstream<ELEMENT, TRAITS, ALLOCATOR> stream(c_str());
            while (std::getline(stream, current, delimiter))
            {
                tokens.push_back(current);
            };

            return tokens;
        }

        int compareNoCase(const ELEMENT *string)
        {
            return getLower().compare(string);
        }

        void format(const ELEMENT *formatting)
        {
            append(formatting);
        }

        template<typename TYPE, typename... ARGUMENTS>
        void format(const ELEMENT *formatting, const TYPE &value, ARGUMENTS... arguments)
        {
            while (formatting && *formatting)
            {
                ELEMENT currentCharacter = *formatting++;
                if (currentCharacter == '%' && formatting && *formatting)
                {
                    ELEMENT nextCharacter = *formatting++;
                    if (nextCharacter == '%')
                    {
                        // %%
                        (*this) += nextCharacter;
                    }
                    else if (nextCharacter == 'v')
                    {
                        // %v
                        (*this) += value;
                        format(formatting, arguments...);
                        return;
                    }
                    else
                    {
                        // %(other)
                        (*this) += currentCharacter;
                        (*this) += nextCharacter;
                    }
                }
                else
                {
                    (*this) += currentCharacter;
                }
            };
        }

        baseString<ELEMENT> &operator = (const ELEMENT &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const bool &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::boolalpha << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const float &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << std::showpoint << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const INT8 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const UINT8 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const INT16 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const UINT16 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const INT32 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const UINT32 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const INT64 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const UINT64 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const HRESULT &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << "0x" << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(4) << std::hex << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const DWORD &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << "0x" << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(4) << std::hex << value;
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const Math::Float2 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ')';
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const Math::Float3 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ')';
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const Math::Float4 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const Math::Color &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.r << ',' << value.g << ',' << value.b << ',' << value.a << ')';
            return static_cast<baseString &>(assign(stream.str()));
        }

        baseString<ELEMENT> &operator = (const Math::Quaternion &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            return static_cast<baseString &>(assign(stream.str()));
        }

        template <typename CONVERSION>
        baseString<ELEMENT> &operator = (const CONVERSION *string)
        {
            if (string)
            {
                converter<ELEMENT, CONVERSION>::convert(*this, string);
            }
            else
            {
                empty();
            }

            return (*this);
        }

        template <typename CONVERSION>
        baseString<ELEMENT> &operator = (const basic_string<CONVERSION> &string)
        {
            if (string.empty())
            {
                empty();
            }
            else
            {
                converter<ELEMENT, CONVERSION>::convert(*this, string.c_str());
            }

            return (*this);
        }

        template <typename CONVERSION>
        baseString<ELEMENT> &operator = (const baseString<CONVERSION> &string)
        {
            if (string.empty())
            {
                empty();
            }
            else
            {
                converter<ELEMENT, CONVERSION>::convert(*this, string.c_str());
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

        void operator += (const INT8 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const UINT8 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const INT16 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const UINT16 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const INT32 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const UINT32 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const INT64 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const UINT64 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << value;
            append(stream.str());
        }

        void operator += (const HRESULT &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << "0x" << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(4) << std::hex << value;
            append(stream.str());
        }

        void operator += (const DWORD &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << "0x" << std::uppercase << std::setfill(ELEMENT('0')) << std::setw(4) << std::hex << value;
            append(stream.str());
        }

        void operator += (const Math::Float2 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ')';
            append(stream.str());
        }

        void operator += (const Math::Float3 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ')';
            append(stream.str());
        }

        void operator += (const Math::Float4 &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            append(stream.str());
        }

        void operator += (const Math::Color &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.r << ',' << value.g << ',' << value.b << ',' << value.a << ')';
            append(stream.str());
        }

        void operator += (const Math::Quaternion &value)
        {
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            append(stream.str());
        }

        template <typename CONVERSION>
        void operator += (const CONVERSION *string)
        {
            append(baseString(string));
        }

        template <typename CONVERSION>
        void operator += (const basic_string<CONVERSION> &string)
        {
            append(baseString(string));
        }

        template <typename CONVERSION>
        void operator += (const baseString<CONVERSION> &string)
        {
            append(baseString(string));
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

        operator INT32 () const
        {
            INT32 value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0 : value);
        }

        operator UINT32 () const
        {
            UINT32 value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0 : value);
        }

        operator INT64 () const
        {
            INT64 value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0 : value);
        }

        operator UINT64 () const
        {
            UINT64 value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> value;
            return (stream.fail() ? 0 : value);
        }

        operator Math::Float2 () const
        {
            ELEMENT separator;
            Math::Float2 value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> separator >> value.x >> separator >> value.y >> separator; // ( X , Y )
            return (stream.fail() ? Math::Float2(0.0f, 0.0f) : value);
        }

        operator Math::Float3 () const
        {
            ELEMENT separator;
            Math::Float3 value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> separator >> value.x >> separator >> value.y >> separator >> value.z >> separator; // ( x , Y , Z )
            return (stream.fail() ? Math::Float3(0.0f, 0.0f, 0.0f) : value);
        }

        operator Math::Float4 () const
        {
            ELEMENT separator;
            Math::Float4 value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> separator >> value.x >> separator >> value.y >> separator >> value.z >> separator >> value.w >> separator; // ( X , Y , Z , W )
            return (stream.fail() ? Math::Float4(0.0f, 0.0f, 0.0f, 0.0f) : value);
        }

        operator Math::Color () const
        {
            Math::Color value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> value.r;
            if (stream.fail())
            {
                ELEMENT separator;
                stream >> separator >> value.r >> separator >> value.g >> separator >> value.b >> separator; // ( R , G , B ) or ( R , G , B ,
                if (!stream.fail())
                {
                    switch (separator)
                    {
                    case ')':
                        value.a = 1.0f;
                        break;

                    case ',':
                        stream >> value.a >> separator;
                        break;
                    };
                }
            }
            else
            {
                value.set(value.r);
            }

            return (stream.fail() ? Math::Color(1.0f, 1.0f, 1.0f, 1.0f) : value);
        }

        operator Math::Quaternion () const
        {
            ELEMENT separator;
            Math::Quaternion value;
            std::basic_stringstream<ELEMENT, std::char_traits<ELEMENT>, std::allocator<ELEMENT>> stream(*this);
            stream >> separator >> value.x >> separator >> value.y >> separator >> value.z >> separator;
            if (!stream.fail())
            {
                switch (separator)
                {
                case ')':
                    value.setEulerRotation(value.x, value.y, value.z);
                    break;

                case ',':
                    stream >> value.w >> separator;
                    break;
                };
            }

            return (stream.fail() ? Math::Quaternion::Identity : value);
        }

        operator const ELEMENT * () const
        {
            return c_str();
        }

        operator std::basic_string<ELEMENT> & ()
        {
            return static_cast<basic_string &>(*this);
        }

        operator const std::basic_string<ELEMENT> & () const
        {
            return static_cast<const basic_string &>(*this);
        }
    };

    typedef baseString<char> string;
    typedef baseString<wchar_t> wstring;
}; // namespace Gek

namespace std
{
    template <>
    struct hash<Gek::string>
    {
        size_t operator()(const Gek::string &value) const
        {
            return hash<string>()(value);
        }
    };

    template <>
    struct hash<Gek::wstring>
    {
        size_t operator()(const Gek::wstring &value) const
        {
            return hash<wstring>()(value);
        }
    };
}; // namespace std