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
        bool get(LPCWSTR expression, INT32 &result, INT32 defaultValue)
        {
            float value;
            bool success = (shuntingYard.evaluate(expression, value) == ShuntingYard::Status::Success);
            result = (success ? INT32(value) : defaultValue);
            return success;
        }

        bool get(LPCWSTR expression, UINT32 &result, UINT32 defaultValue)
        {
            float value;
            bool success = (shuntingYard.evaluate(expression, value) == ShuntingYard::Status::Success);
            result = (success ? UINT32(value) : defaultValue);
            return success;
        }

        bool get(LPCWSTR expression, INT64 &result, INT64 defaultValue)
        {
            float value;
            bool success = (shuntingYard.evaluate(expression, value) == ShuntingYard::Status::Success);
            result = (success ? INT64(value) : defaultValue);
            return success;
        }

        bool get(LPCWSTR expression, UINT64 &result, UINT64 defaultValue)
        {
            float value;
            bool success = (shuntingYard.evaluate(expression, value) == ShuntingYard::Status::Success);
            result = (success ? UINT64(value) : defaultValue);
            return success;
        }

        bool get(LPCWSTR expression, float &result, float defaultValue)
        {
            float value;
            bool success = (shuntingYard.evaluate(expression, value) == ShuntingYard::Status::Success);
            result = (success ? value : defaultValue);
            return success;
        }

        bool get(LPCWSTR expression, Gek::Math::Float2 &result, const Gek::Math::Float2 &defaultValue)
        {
            Gek::Math::Float2 value;
            bool success = (shuntingYard.evaluate(expression, value) == ShuntingYard::Status::Success);
            result = (success ? value : defaultValue);
            return success;
        }

        bool get(LPCWSTR expression, Gek::Math::Float3 &result, const Gek::Math::Float3 &defaultValue)
        {
            Gek::Math::Float3 value;
            bool success = (shuntingYard.evaluate(expression, value) == ShuntingYard::Status::Success);
            result = (success ? value : defaultValue);
            return success;
        }

        bool get(LPCWSTR expression, Gek::Math::Float4 &result, const Gek::Math::Float4 &defaultValue)
        {
            Gek::Math::Float4 value;
            bool success = (shuntingYard.evaluate(expression, value) == ShuntingYard::Status::Success);
            result = (success ? value : defaultValue);
            return success;
        }

        bool get(LPCWSTR expression, Gek::Math::Color &result, const Gek::Math::Color &defaultValue)
        {
            bool success = (shuntingYard.evaluate(expression, result) == ShuntingYard::Status::Success);
            if (!success)
            {
                Math::Float3 rgbValue;
                success = (shuntingYard.evaluate(expression, rgbValue) == ShuntingYard::Status::Success);
                if (success)
                {
                    result.set(rgbValue);
                }
            }

            if (!success)
            {
                result = defaultValue;
            }

            return success;
        }

        bool get(LPCWSTR expression, Gek::Math::Quaternion &result, const Gek::Math::Quaternion &defaultValue)
        {
            bool success = (shuntingYard.evaluate(expression, result) == ShuntingYard::Status::Success);
            if (!success)
            {
                Math::Float3 euler;
                success = (shuntingYard.evaluate(expression, euler) == ShuntingYard::Status::Success);
                if (success)
                {
                    result.setEulerRotation(euler.x, euler.y, euler.z);
                }
            }

            if (!success)
            {
                result = defaultValue;
            }

            return success;
        }

        bool get(LPCWSTR expression, CStringW &result)
        {
            result = expression;
            return true;
        }
    }; // namespace Evaluator
}; // namespace Gek
