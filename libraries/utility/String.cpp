#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"
#include <locale>
#include <vector>

namespace Gek
{
    namespace String
    {
        template <typename TYPE>
        struct csv_reader : std::ctype<TYPE>
        {
            csv_reader() : std::ctype<TYPE>(get_table())
            {
            }

            static std::ctype_base::mask const* get_table()
            {
                static std::vector<std::ctype_base::mask> rc(table_size, std::ctype_base::mask());

                rc[','] = std::ctype_base::space;
                rc['\n'] = std::ctype_base::space;
                rc[' '] = std::ctype_base::space;
                return &rc[0];
            }
        };

        CStringW from(double value)
        {
            return format(L"%f", value);
        }

        CStringW from(float value)
        {
            return format(L"%f", value);
        }

        CStringW from(const Gek::Math::Float2 &value)
        {
            return format(L"%f,%f", value.x, value.y);
        }

        CStringW from(const Gek::Math::Float3 &value)
        {
            return format(L"%f,%f,%f", value.x, value.y, value.z);
        }

        CStringW from(const Gek::Math::Float4 &value)
        {
            return format(L"%f,%f,%f,%f", value.x, value.y, value.z, value.w);
        }

        CStringW from(const Gek::Math::Color &value)
        {
            return format(L"%f,%f,%f,%f", value.r, value.g, value.b, value.a);
        }

        CStringW from(const Gek::Math::Quaternion &value)
        {
            return format(L"%f,%f,%f,%f", value.x, value.y, value.z, value.w);
        }

        CStringW from(INT8 value)
        {
            return format(L"%hhd", value);
        }

        CStringW from(UINT8 value)
        {
            return format(L"%hhu", value);
        }

        CStringW from(INT16 value)
        {
            return format(L"%hd", value);
        }

        CStringW from(UINT16 value)
        {
            return format(L"%hu", value);
        }

        CStringW from(INT32 value)
        {
            return format(L"%d", value);
        }

        CStringW from(UINT32 value)
        {
            return format(L"%u", value);
        }

        CStringW from(DWORD value)
        {
            return format(L"0x%08X", value);
        }

        CStringW from(LPCVOID value)
        {
            return format(L"%p", value);
        }

        CStringW from(INT64 value)
        {
            return format(L"%lld", value);
        }

        CStringW from(UINT64 value)
        {
            return format(L"%llu", value);
        }

        CStringW from(bool value)
        {
            return (value ? L"true" : L"false");
        }

        CStringW from(LPCSTR value, bool fromUTF8)
        {
            return LPCWSTR(CA2W(value, fromUTF8 ? CP_UTF8 : CP_ACP));
        }

        CStringW from(LPCWSTR value)
        {
            return value;
        }

        CStringW from(const CStringW &value)
        {
            return value;
        }

        CStringA format(LPCSTR format, ...)
        {
            CStringA result;
            if (format != nullptr)
            {
                va_list variableList;
                va_start(variableList, format);
                result.FormatV(format, variableList);
                va_end(variableList);
            }

            return result;
        }

        CStringW format(LPCWSTR format, ...)
        {
            CStringW result;
            if (format != nullptr)
            {
                va_list variableList;
                va_start(variableList, format);
                result.FormatV(format, variableList);
                va_end(variableList);
            }

            return result;
        }
    }; // namespace String
}; // namespace Gek
