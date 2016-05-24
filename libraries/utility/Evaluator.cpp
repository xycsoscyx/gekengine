#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\ShuntingYard.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"
#include <unordered_map>
#include <functional>
#include <random>
#include <stack>
#include <regex>

namespace Gek
{
    namespace Evaluator
    {
        static ShuntingYard shuntingYard;

        template <typename TYPE>
        void castResult(const wchar_t *expression, TYPE &result, TYPE defaultValue)
        {
            try
            {
                float value;
                shuntingYard.evaluate(expression, value);
                result = TYPE(value);
            }
            catch (ShuntingYard::Exception exception)
            {
                result = defaultValue;
            };
        }

        template <typename TYPE>
        void getResult(const wchar_t *expression, TYPE &result, const TYPE &defaultValue)
        {
            try
            {
                shuntingYard.evaluate(expression, result);
            }
            catch (ShuntingYard::Exception exception)
            {
                result = defaultValue;
            };
        }

        void get(const wchar_t *expression, INT32 &result, INT32 defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wchar_t *expression, UINT32 &result, UINT32 defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wchar_t *expression, INT64 &result, INT64 defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wchar_t *expression, UINT64 &result, UINT64 defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wchar_t *expression, float &result, float defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wchar_t *expression, Gek::Math::Float2 &result, const Gek::Math::Float2 &defaultValue)
        {
            getResult(expression, result, defaultValue);
        }

        void get(const wchar_t *expression, Gek::Math::Float3 &result, const Gek::Math::Float3 &defaultValue)
        {
            getResult(expression, result, defaultValue);
        }

        void get(const wchar_t *expression, Gek::Math::Float4 &result, const Gek::Math::Float4 &defaultValue)
        {
            getResult(expression, result, defaultValue);
        }

        void get(const wchar_t *expression, Gek::Math::Color &result, const Gek::Math::Color &defaultValue)
        {
            try
            {
                shuntingYard.evaluate(expression, result);
            }
            catch (ShuntingYard::Exception exception)
            {
                try
                {
                    Math::Float3 rgbValue;
                    shuntingYard.evaluate(expression, rgbValue);
                    result.set(rgbValue);
                }
                catch (ShuntingYard::Exception exception)
                {
                    result = defaultValue;
                };
            };
        }

        void get(const wchar_t *expression, Gek::Math::Quaternion &result, const Gek::Math::Quaternion &defaultValue)
        {
            try
            {
                shuntingYard.evaluate(expression, result);
            }
            catch (ShuntingYard::Exception exception)
            {
                try
                {
                    Math::Float3 euler;
                    shuntingYard.evaluate(expression, euler);
                    result.setEulerRotation(euler.x, euler.y, euler.z);
                }
                catch (ShuntingYard::Exception exception)
                {
                    result = defaultValue;
                };
            };
        }

        void get(const wchar_t *expression, wstring &result)
        {
            result = expression;
        }
    }; // namespace Evaluator
}; // namespace Gek
