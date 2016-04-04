#pragma once

#include "GEK\Math\Vector2.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Utility\Hash.h"
#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <sstream>

namespace Gek
{
    namespace String
    {
        template <typename CHAR>
        bool to(const CHAR *expression, double &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, float &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Float2 &value)
        {
            CHAR comma;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value.x >> comma >> value.y;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Float3 &value)
        {
            CHAR comma;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value.x >> comma >> value.y >> comma >> value.z >> comma;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Float4 &value)
        {
            CHAR comma;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value.x >> comma >> value.y >> comma >> value.z >> comma >> value.w;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Color &value)
        {
            CHAR comma;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value.r >> comma >> value.g >> comma >> value.b;
            bool failed = stream.fail();
            if (!failed)
            {
                value.a = 1.0f;
                stream >> comma >> value.a;
            }

            return failed;
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Quaternion &value)
        {
            CHAR comma;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value.x >> comma >> value.y >> comma >> value.z;
            bool failed = stream.fail();
            if (!failed)
            {
                stream >> comma >> value.w;
                if (stream.fail())
                {
                    value.setEulerRotation(value.x, value.y, value.z);
                }
            }

            return failed;
        }

        template <typename CHAR>
        bool to(const CHAR *expression, INT32 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, UINT32 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, INT64 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, UINT64 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, bool &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> std::boolalpha >> value;
            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> &value)
        {
            value = expression;
            return true;
        }

        template <typename CHAR>
        bool to(const CHAR *expression, std::basic_string<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> &value)
        {
            value = expression;
            return true;
        }

        template <typename TYPE, typename CHAR>
        TYPE to(const CHAR *expression)
        {
            TYPE value = {};
            to(expression, value);
            return value;
        }

        template <typename TYPE, typename CHAR>
        TYPE to(const CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> &expression)
        {
            TYPE value = {};
            to(expression.GetString(), value);
            return value;
        }

        template <typename TYPE, typename CHAR>
        TYPE to(const std::basic_string<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> &expression)
        {
            TYPE value = {};
            to(expression.data(), value);
            return value;
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(double value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(float value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Float2 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value.x << ',' << value.y;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Float3 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value.x << ',' << value.y << ',' << value.z;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Float4 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value.x << ',' << value.y << ',' << value.z << ',' << value.w;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Color &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value.r << ',' << value.g << ',' << value.b << ',' << value.a;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Quaternion &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value.x << ',' << value.y << ',' << value.z << ',' << value.w;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(INT8 value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(UINT8 value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(INT16 value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(UINT16 value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(INT32 value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(UINT32 value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(DWORD value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(LPCVOID value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(INT64 value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(UINT64 value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(bool value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << std::boolalpha << value;
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(LPCSTR value)
        {
            if (std::is_same<CHAR, wchar_t>::value)
            {
                return CA2W(value, CP_UTF8).m_psz;
            }
            else
            {
                return value;
            }
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(LPCWSTR value)
        {
            if (std::is_same<CHAR, char>::value)
            {
                return CW2A(value, CP_UTF8).m_psz;
            }
            else
            {
                return value;
            }
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const GUID &value)
        {
            return CComBSTR(value).m_str;
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> format(const CHAR *formatting)
        {
            CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> result;
            while (*formatting)
            {
                if (*formatting == '%')
                {
                    ++formatting;
                    if (*formatting == '%')
                    {
                        result += *formatting++;
                    }
                    else
                    {
                        throw std::runtime_error("invalid format string: missing arguments");
                    }
                }
                else
                {
                    result += *formatting++;
                }
            };

            return result;
        }

        template<typename CHAR, typename TYPE, typename... ARGS>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> format(const CHAR *formatting, TYPE value, ARGS... args)
        {
            CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> result;
            while (*formatting)
            {
                if (*formatting == '%')
                {
                    ++formatting;
                    if (*formatting == '%')
                    {
                        result += *formatting++;
                    }
                    else
                    {
                        ++formatting;
                        // Should verify format type here
                        result += from<CHAR>(value).GetString();
                        result += format(formatting, args...);
                        return result;
                    }
                }
                else
                {
                    result += *formatting++;
                }
            }
            
            throw std::logic_error("extra arguments provided to format");
        }
    }; // namespace String

    template<typename CHAR, typename... ARGS>
    class Exception
    {
    public:
        LPCSTR file;
        UINT32 line;
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> message;

    public:
        Exception(LPCSTR file, UINT32 line, const CHAR *format, ARGS... args)
            : file(file)
            , line(line)
        {
            message = String::format(format, args...);
        }
    };
}; // namespace Gek

#define GEKEXCEPTION(FORMAT, ...)  throw Gek::Exception<std::remove_const<std::remove_reference<decltype(FORMAT[0])>::type>::type>(__FILE__, __LINE__, FORMAT, __VA_ARGS__)
