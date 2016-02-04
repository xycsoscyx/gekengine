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
        bool get(LPCWSTR expression, INT32 &result)
        {
            float value = 0.0f;
            bool success = get(expression, value);
            if(success)
            {
                result = INT32(value);
            }

            return success;
        }

        bool get(LPCWSTR expression, UINT32 &result)
        {
            float value = 0.0f;
            bool success = get(expression, value);
            if (success)
            {
                result = UINT32(value);
            }

            return success;
        }

        bool get(LPCWSTR expression, INT64 &result)
        {
            float value = 0.0f;
            bool success = get(expression, value);
            if (success)
            {
                result = INT64(value);
            }

            return success;
        }

        bool get(LPCWSTR expression, UINT64 &result)
        {
            float value = 0.0f;
            bool success = get(expression, value);
            if (success)
            {
                result = UINT64(value);
            }

            return success;
        }

        bool get(LPCWSTR expression, float &result)
        {
            return (shuntingYard.evaluate(expression, result) == ShuntingYard::Status::Success);
        }

        bool get(LPCWSTR expression, Gek::Math::Float2 &result)
        {
            return (shuntingYard.evaluate(expression, result) == ShuntingYard::Status::Success);
        }

        bool get(LPCWSTR expression, Gek::Math::Float3 &result)
        {
            return (shuntingYard.evaluate(expression, result) == ShuntingYard::Status::Success);
        }

        bool get(LPCWSTR expression, Gek::Math::Float4 &result)
        {
            return (shuntingYard.evaluate(expression, result) == ShuntingYard::Status::Success);
        }

        bool get(LPCWSTR expression, Gek::Math::Color &result)
        {
            bool success = (shuntingYard.evaluate(expression, result) == ShuntingYard::Status::Success);
            if (!success)
            {
                Math::Float3 rgb;
                success = (shuntingYard.evaluate(expression, rgb) == ShuntingYard::Status::Success);
                if (success)
                {
                    result.set(rgb);
                }
            }

            return success;
        }

        bool get(LPCWSTR expression, Gek::Math::Quaternion &result)
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

            return success;
        }
    }; // namespace Evaluator
}; // namespace Gek
