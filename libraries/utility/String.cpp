#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"

namespace Gek
{
    namespace String
    {
        bool to(LPCWSTR expression, double &value)
        {
            value = 0.0;
            return (swscanf_s(expression, L"%lf", &value) == 1);
        }

        bool to(LPCWSTR expression, float &value)
        {
            value = 0.0f;
            return (swscanf_s(expression, L"%f", &value) == 1);
        }

        bool to(LPCWSTR expression, Gek::Math::Float2 &value)
        {
            return (swscanf_s(expression, L"%f,%f", &value.x, &value.y) == 2);
        }

        bool to(LPCWSTR expression, Gek::Math::Float3 &value)
        {
            return (swscanf_s(expression, L"%f,%f,%f", &value.x, &value.y, &value.z) == 3);
        }

        bool to(LPCWSTR expression, Gek::Math::Float4 &value)
        {
            return (swscanf_s(expression, L"%f,%f,%f,%f", &value.x, &value.y, &value.z, &value.w) == 4);
        }

        bool to(LPCWSTR expression, Gek::Math::Quaternion &value)
        {
            int values = swscanf_s(expression, L"%f,%f,%f,%f", &value.x, &value.y, &value.z, &value.w);
            if (values == 3)
            {
                value.setEulerRotation(value.x, value.y, value.z);
                values = 4;
            }

            return (values == 4);
        }

        bool to(LPCWSTR expression, INT32 &value)
        {
            value = 0;
            return (swscanf_s(expression, L"%d", &value) == 1);
        }

        bool to(LPCWSTR expression, UINT32 &value)
        {
            value = 0;
            return (swscanf_s(expression, L"%u", &value) == 1);
        }

        bool to(LPCWSTR expression, INT64 &value)
        {
            value = 0;
            return (swscanf_s(expression, L"%lld", &value) == 1);
        }

        bool to(LPCWSTR expression, UINT64 &value)
        {
            value = 0;
            return (swscanf_s(expression, L"%llu", &value) == 1);
        }

        bool to(LPCWSTR expression, bool &value)
        {
            if (_wcsicmp(expression, L"true") == 0 ||
                _wcsicmp(expression, L"yes") == 0)
            {
                value = true;
                return true;
            }

            INT32 integerValue = 0;
            if (to(expression, integerValue))
            {
                value = (integerValue == 0 ? false : true);
                return true;
            }

            return false;
        }

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
