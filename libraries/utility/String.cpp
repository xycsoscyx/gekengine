#include "GEK\Utility\String.h"
#include "GEK\Utility\Evaluator.h"

namespace Gek
{
    namespace String
    {
        double getDouble(LPCWSTR expression)
        {
            double value = 0.0;
            Evaluator::getDouble(expression, value);
            return value;
        }

        float getFloat(LPCWSTR expression)
        {
            float value = 0.0f;
            Evaluator::getFloat(expression, value);
            return value;
        }

        Gek::Math::Float2 getFloat2(LPCWSTR expression)
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

        Gek::Math::Float3 getFloat3(LPCWSTR expression)
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

        Gek::Math::Float4 getFloat4(LPCWSTR expression)
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

        Gek::Math::Quaternion getQuaternion(LPCWSTR expression)
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

        INT32 getINT32(LPCWSTR expression)
        {
            INT32 value = 0;
            Evaluator::getINT32(expression, value);
            return value;
        }

        UINT32 getUINT32(LPCWSTR expression)
        {
            UINT32 value = 0;
            Evaluator::getUINT32(expression, value);
            return value;
        }

        INT64 getINT64(LPCWSTR expression)
        {
            INT64 value = 0;
            Evaluator::getINT64(expression, value);
            return value;
        }

        UINT64 getUINT64(LPCWSTR expression)
        {
            UINT64 value = 0;
            Evaluator::getUINT64(expression, value);
            return value;
        }

        bool getBoolean(LPCWSTR expression)
        {
            bool value = false;
            Evaluator::getBoolean(expression, value);
            return value;
        }

        CStringW setDouble(double value)
        {
            CStringW strValue;
            strValue.Format(L"%f", value);
            return strValue;
        }

        CStringW setFloat(float value)
        {
            CStringW strValue;
            strValue.Format(L"%f", value);
            return strValue;
        }

        CStringW setFloat2(Gek::Math::Float2 value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f", value.x, value.y);
            return strValue;
        }

        CStringW setFloat3(Gek::Math::Float3 value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f", value.x, value.y, value.z);
            return strValue;
        }

        CStringW setFloat4(Gek::Math::Float4 value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f,%f", value.x, value.y, value.z, value.w);
            return strValue;
        }

        CStringW setQuaternion(Gek::Math::Quaternion value)
        {
            CStringW strValue;
            strValue.Format(L"%f,%f,%f,%f", value.x, value.y, value.z, value.w);
            return strValue;
        }

        CStringW setINT32(INT32 value)
        {
            CStringW strValue;
            strValue.Format(L"%d", value);
            return strValue;
        }

        CStringW setUINT32(UINT32 value)
        {
            CStringW strValue;
            strValue.Format(L"%u", value);
            return strValue;
        }

        CStringW setINT64(INT64 value)
        {
            CStringW strValue;
            strValue.Format(L"%lld", value);
            return strValue;
        }

        CStringW setUINT64(UINT64 value)
        {
            CStringW strValue;
            strValue.Format(L"%llu", value);
            return strValue;
        }

        CStringW setBoolean(bool value)
        {
            return (value ? L"true" : L"false");
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
