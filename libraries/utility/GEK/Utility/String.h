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
        bool to(const CHAR *expression, double &value, double defaultValue = 0.0)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, float &value, float defaultValue = 0.0f)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Float2 &value, const Gek::Math::Float2 &defaultValue = Gek::Math::Float2(0.0f, 0.0f))
        {
            CHAR separator;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> separator >> value.x >> separator >> value.y >> separator; // ( X , Y )
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Float3 &value, const Gek::Math::Float3 &defaultValue = Gek::Math::Float3(0.0f, 0.0f, 0.0f))
        {
            CHAR separator;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> separator >> value.x >> separator >> value.y >> separator >> value.z >> separator; // ( x , Y , Z )
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Float4 &value, const Gek::Math::Float4 &defaultValue = Gek::Math::Float4(0.0f, 0.0f, 0.0f, 0.0f))
        {
            CHAR separator;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> separator >> value.x >> separator >> value.y >> separator >> value.z >> separator >> value.w >> separator; // ( X , Y , Z , W )
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Color &value, const Gek::Math::Color &defaultValue = Gek::Math::Color(0.0f, 0.0f, 0.0f, 1.0f))
        {
            CHAR separator;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> separator >> value.r >> separator >> value.g >> separator >> value.b >> separator; // ( R , G , B ) or ( R , G , B ,
            bool failed = stream.fail();
            if (!failed)
            {
                switch (separator)
                {
                case ')':
                    value.a = 1.0f;
                    break;

                case ',':
                    stream >> value.a >> separator;
                    failed = stream.fail();
                    break;
                };
            }

            if (failed)
            {
                value = defaultValue;
            }

            return !failed;
        }

        template <typename CHAR>
        bool to(const CHAR *expression, Gek::Math::Quaternion &value, const Gek::Math::Quaternion &defaultValue = Gek::Math::Quaternion::Identity)
        {
            CHAR separator;
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> separator >> value.x >> separator >> value.y >> separator >> value.z >> separator;
            bool failed = stream.fail();
            if (!failed)
            {
                switch (separator)
                {
                case ')':
                    value.setEulerRotation(value.x, value.y, value.z);
                    break;

                case ',':
                    stream >> value.w >> separator;
                    failed = stream.fail();
                    break;
                };
            }

            if (failed)
            {
                value = defaultValue;
            }

            return !failed;
        }

        template <typename CHAR>
        bool to(const CHAR *expression, INT32 &value, INT32 defaultValue = 0)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, UINT32 &value, UINT32 defaultValue = 0)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, INT64 &value, INT64 defaultValue = 0)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, UINT64 &value, UINT64 defaultValue = 0)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> value;
            if (stream.fail())
            {
                value = defaultValue;
            }

            return !stream.fail();
        }

        template <typename CHAR>
        bool to(const CHAR *expression, bool &value, bool defaultValue = false)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream(expression);
            stream >> std::boolalpha >> value;
            if (stream.fail())
            {
                value = defaultValue;
            }

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
            to(expression, value);
            return value;
        }

        template <typename TYPE, typename CHAR>
        TYPE to(const CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> &expression)
        {
            TYPE value;
            to(expression.GetString(), value);
            return value;
        }

        template <typename TYPE, typename CHAR>
        TYPE to(const CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> &expression, TYPE defaultValue)
        {
            TYPE value;
            to(expression.GetString(), value, defaultValue);
            return value;
        }

        template <typename TYPE, typename CHAR>
        TYPE to(const std::basic_string<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> &expression)
        {
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
            stream << '(' << value.x << ',' << value.y << ')';
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Float3 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ')';
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Float4 &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Color &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << '(' << value.r << ',' << value.g << ',' << value.b << ',' << value.a << ')';
            return stream.str().c_str();
        }

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> from(const Gek::Math::Quaternion &value)
        {
            std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
            stream << '(' << value.x << ',' << value.y << ',' << value.z << ',' << value.w << ')';
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
}; // namespace Gek
