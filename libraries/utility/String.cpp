#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"

namespace Gek
{
    namespace String
    {
        void to(LPCWSTR expression, double &value)
        {
            value = 0.0;
            Evaluator::get(expression, value);
        }

        void to(LPCWSTR expression, float &value)
        {
            value = 0.0f;
            Evaluator::get(expression, value);
        }

        void to(LPCWSTR expression, Gek::Math::Float2 &value)
        {
            if (!Evaluator::get(expression, value))
            {
                if (Evaluator::get(expression, value.x))
                {
                    value.y = value.x;
                }
            }
        }

        void to(LPCWSTR expression, Gek::Math::Float3 &value)
        {
            if (!Evaluator::get(expression, value))
            {
                if (Evaluator::get(expression, value.x))
                {
                    value.y = value.z = value.x;
                }
            }
        }

        void to(LPCWSTR expression, Gek::Math::Float4 &value)
        {
            if (!Evaluator::get(expression, value))
            {
                if (Evaluator::get(expression, *(Gek::Math::Float3 *)&value))
                {
                    value.w = 1.0f;
                }
                else
                {
                    if (Evaluator::get(expression, value.x))
                    {
                        value.y = value.z = value.w = value.x;
                    }
                }
            }
        }

        void to(LPCWSTR expression, Gek::Math::Quaternion &value)
        {
            if (!Evaluator::get(expression, value))
            {
                Gek::Math::Float3 euler;
                if (Evaluator::get(expression, euler))
                {
                    value.setEuler(euler);
                }
            }
        }

        void to(LPCWSTR expression, INT32 &value)
        {
            value = 0;
            Evaluator::get(expression, value);
        }

        void to(LPCWSTR expression, UINT32 &value)
        {
            value = 0;
            Evaluator::get(expression, value);
        }

        void to(LPCWSTR expression, INT64 &value)
        {
            value = 0;
            Evaluator::get(expression, value);
        }

        void to(LPCWSTR expression, UINT64 &value)
        {
            value = 0;
            Evaluator::get(expression, value);
        }

        void to(LPCWSTR expression, bool &value)
        {
            value = false;
            Evaluator::get(expression, value);
        }

        CStringW from(double value)
        {
            CStringW strValue;
            strValue.Format(L"%f", value);
            return strValue;
        }

        CStringW from(float value)
        {
            CStringW strValue;
            strValue.Format(L"%f", value);
            return strValue;
        }

        CStringW from(const Gek::Math::Float2 &value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f", value.x, value.y);
            return strValue;
        }

        CStringW from(const Gek::Math::Float3 &value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f", value.x, value.y, value.z);
            return strValue;
        }

        CStringW from(const Gek::Math::Float4 &value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f,%f", value.x, value.y, value.z, value.w);
            return strValue;
        }

        CStringW from(const Gek::Math::Quaternion &value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f,%f", value.x, value.y, value.z, value.w);
            return strValue;
        }

        CStringW from(INT8 value)
        {
            CStringW strValue;
            strValue.Format(L"%hhd", value);
            return strValue;
        }

        CStringW from(UINT8 value)
        {
            CStringW strValue;
            strValue.Format(L"%hhu", value);
            return strValue;
        }

        CStringW from(INT16 value)
        {
            CStringW strValue;
            strValue.Format(L"%hd", value);
            return strValue;
        }

        CStringW from(UINT16 value)
        {
            CStringW strValue;
            strValue.Format(L"%hu", value);
            return strValue;
        }

        CStringW from(INT32 value)
        {
            CStringW strValue;
            strValue.Format(L"%d", value);
            return strValue;
        }

        CStringW from(UINT32 value)
        {
            CStringW strValue;
            strValue.Format(L"%u", value);
            return strValue;
        }

        CStringW from(DWORD value)
        {
            CStringW strValue;
            strValue.Format(L"0x%08X", value);
            return strValue;
        }

        CStringW from(LPCVOID value)
        {
            CStringW strValue;
            strValue.Format(L"%p", value);
            return strValue;
        }

        CStringW from(INT64 value)
        {
            CStringW strValue;
            strValue.Format(L"%lld", value);
            return strValue;
        }

        CStringW from(UINT64 value)
        {
            CStringW strValue;
            strValue.Format(L"%llu", value);
            return strValue;
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
