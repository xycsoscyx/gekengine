#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"

namespace Gek
{
    namespace String
    {
        double toDouble(LPCWSTR expression)
        {
            double value = 0.0;
            Evaluator::getDouble(expression, value);
            return value;
        }

        float toFloat(LPCWSTR expression)
        {
            float value = 0.0f;
            Evaluator::getFloat(expression, value);
            return value;
        }

        Gek::Math::Float2 toFloat2(LPCWSTR expression)
        {
            Gek::Math::Float2 vector;
            if (!Evaluator::getFloat2(expression, vector))
            {
                if (Evaluator::getFloat(expression, vector.x))
                {
                    vector.y = vector.x;
                }
            }

            return vector;
        }

        Gek::Math::Float3 toFloat3(LPCWSTR expression)
        {
            Gek::Math::Float3 vector;
            if (!Evaluator::getFloat3(expression, vector))
            {
                if (Evaluator::getFloat(expression, vector.x))
                {
                    vector.y = vector.z = vector.x;
                }
            }

            return vector;
        }

        Gek::Math::Float4 toFloat4(LPCWSTR expression)
        {
            Gek::Math::Float4 vector;
            if (!Evaluator::getFloat4(expression, vector))
            {
                if (Evaluator::getFloat3(expression, *(Gek::Math::Float3 *)&vector))
                {
                    vector.w = 1.0f;
                }
                else
                {
                    if (Evaluator::getFloat(expression, vector.x))
                    {
                        vector.y = vector.z = vector.w = vector.x;
                    }
                }
            }

            return vector;
        }

        Gek::Math::Quaternion toQuaternion(LPCWSTR expression)
        {
            Gek::Math::Quaternion rotation;
            if (!Evaluator::getQuaternion(expression, rotation))
            {
                Gek::Math::Float3 euler;
                if (Evaluator::getFloat3(expression, euler))
                {
                    rotation.setEuler(euler);
                }
            }

            return rotation;
        }

        INT32 toINT32(LPCWSTR expression)
        {
            INT32 value = 0;
            Evaluator::getINT32(expression, value);
            return value;
        }

        UINT32 toUINT32(LPCWSTR expression)
        {
            UINT32 value = 0;
            Evaluator::getUINT32(expression, value);
            return value;
        }

        INT64 toINT64(LPCWSTR expression)
        {
            INT64 value = 0;
            Evaluator::getINT64(expression, value);
            return value;
        }

        UINT64 toUINT64(LPCWSTR expression)
        {
            UINT64 value = 0;
            Evaluator::getUINT64(expression, value);
            return value;
        }

        bool toBoolean(LPCWSTR expression)
        {
            bool value = false;
            Evaluator::getBoolean(expression, value);
            return value;
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
