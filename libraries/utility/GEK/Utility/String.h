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

        CStringW from(double value);
        CStringW from(float value);
        CStringW from(const Gek::Math::Float2 &value);
        CStringW from(const Gek::Math::Float3 &value);
        CStringW from(const Gek::Math::Float4 &value);
        CStringW from(const Gek::Math::Color &value);
        CStringW from(const Gek::Math::Quaternion &value);
        CStringW from(INT8 value);
        CStringW from(UINT8 value);
        CStringW from(INT16 value);
        CStringW from(UINT16 value);
        CStringW from(INT32 value);
        CStringW from(UINT32 value);
        CStringW from(DWORD value);
        CStringW from(LPCVOID value);
        CStringW from(INT64 value);
        CStringW from(UINT64 value);
        CStringW from(bool value);
        CStringW from(LPCSTR value, bool fromUTF8 = false);
        CStringW from(LPCWSTR value);
        CStringW from(const CStringW &value);
        CStringW from(const GUID &value);

        template <typename CHAR>
        CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> format(const CHAR *formatting)
        {
            CStringT<CHAR, StrTraitATL<CHAR, ChTraitsCRT<CHAR>>> result;
            while (*formatting)
            {
                if (*formatting == '%')
                {
                    if (*(formatting + 1) == '%')
                    {
                        ++formatting;
                    }
                    else
                    {
                        throw std::runtime_error("invalid format string: missing arguments");
                    }
                }

                result += *formatting++;
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
                    if (*(formatting + 1) == '%')
                    {
                        ++formatting;
                    }
                    else
                    {
                        std::basic_stringstream<CHAR, std::char_traits<CHAR>, std::allocator<CHAR>> stream;
                        stream << value;
                        result += stream.str().c_str();
                        result += format(formatting + 1, args...);
                        return result;
                    }
                }

                result += *formatting++;
            }
            
            throw std::logic_error("extra arguments provided to printf");
        }
    }; // namespace String
}; // namespace Gek
