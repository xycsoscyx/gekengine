#include "exprtk.hpp"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"
#include <random>

namespace Gek
{
    namespace Evaluator
    {
        static std::random_device randomDevice;
        static std::mt19937 mersineTwister(randomDevice());

        template <typename TYPE>
        struct RandomFunction : public exprtk::ifunction<TYPE>
        {
            std::uniform_real_distribution<TYPE> uniformRealDistribution;
            RandomFunction(TYPE minimum, TYPE maximum)
                : exprtk::ifunction<TYPE>(1)
                , uniformRealDistribution(minimum, maximum)
            {
            }

            inline TYPE operator()(const TYPE& range)
            {
                return (range * uniformRealDistribution(mersineTwister));
            }
        };

        template <typename TYPE>
        struct LerpFunction : public exprtk::ifunction<TYPE>
        {
            LerpFunction(void) : exprtk::ifunction<TYPE>(3)
            {
            }

            inline TYPE operator()(const TYPE& x, const TYPE& y, const TYPE& step)
            {
                return Math::lerp(x, y, step);
            }
        };

        template <typename TYPE>
        class EquationEvaluator
        {
        private:
            RandomFunction<TYPE> signedRandomFunction;
            RandomFunction<TYPE> unsignedRandomFunction;
            LerpFunction<TYPE> lerpFunction;
            exprtk::symbol_table<TYPE> symbolTable;
            exprtk::expression<TYPE> expression;
            exprtk::parser<TYPE> parser;

        public:
            EquationEvaluator(void)
                : signedRandomFunction(TYPE(-1), 1.0f)
                , unsignedRandomFunction(0.0f, 1.0f)
            {
                symbolTable.add_function("rand", signedRandomFunction);
                symbolTable.add_function("arand", unsignedRandomFunction);
                symbolTable.add_function("lerp", lerpFunction);

                symbolTable.add_constants();
                expression.register_symbol_table(symbolTable);
            }

            bool getValue(LPCWSTR equation, TYPE &value)
            {
                symbolTable.remove_vector("value");
                if (parser.compile(LPCSTR(CW2A(equation)), expression))
                {
                    value = expression.value();
                    return true;
                }
                else
                {
                    return false;
                }
            }

            template <std::size_t SIZE>
            bool getVector(LPCWSTR equation, TYPE(&vector)[SIZE])
            {
                CStringA value;
                value.Format("var vector[%d] := {%S}; value <=> vector;", SIZE, equation);

                symbolTable.remove_vector("value");
                symbolTable.add_vector("value", vector);
                if (parser.compile(value.GetString(), expression))
                {
                    expression.value();
                    return true;
                }
                else
                {
                    return false;
                }
            }
        };

        static EquationEvaluator<float> evaluateFloat;
        static EquationEvaluator<double> evaluateDouble;

        bool get(LPCWSTR expression, double &result)
        {
            return evaluateDouble.getValue(expression, result);
        }

        bool get(LPCWSTR expression, float &result)
        {
            return evaluateFloat.getValue(expression, result);
        }

        bool get(LPCWSTR expression, Gek::Math::Float2 &result)
        {
            return evaluateFloat.getVector(expression, result.data);
        }

        bool get(LPCWSTR expression, Gek::Math::Float3 &result)
        {
            return evaluateFloat.getVector(expression, result.data);
        }

        bool get(LPCWSTR expression, Gek::Math::Float4 &result)
        {
            bool success = evaluateFloat.getVector(expression, result.data);
            if (!success)
            {
                Math::Float3 part;
                success = evaluateFloat.getVector(expression, part.data);
                if (success)
                {
                    result = part.w(1.0f);
                }
            }

            return success;
        }

        bool get(LPCWSTR expression, Gek::Math::Quaternion &result)
        {
            bool success = evaluateFloat.getVector(expression, result.data);
            if (!success)
            {
                Math::Float3 euler;
                success = evaluateFloat.getVector(expression, euler.data);
                if (success)
                {
                    result.setEulerRotation(euler.x, euler.y, euler.z);
                }
            }

            return success;
        }

        bool get(LPCWSTR expression, INT32 &result)
        {
            float value = 0.0f;
            if (get(expression, value))
            {
                result = INT32(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool get(LPCWSTR expression, UINT32 &result)
        {
            float value = 0.0f;
            if (get(expression, value))
            {
                result = UINT32(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool get(LPCWSTR expression, INT64 &result)
        {
            double value = 0.0;
            if (get(expression, value))
            {
                result = INT64(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool get(LPCWSTR expression, UINT64 &result)
        {
            double value = 0.0;
            if (get(expression, value))
            {
                result = UINT64(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool get(LPCWSTR expression, bool &result)
        {
            if (_wcsicmp(expression, L"true") == 0 ||
                _wcsicmp(expression, L"yes") == 0)
            {
                result = true;
                return true;
            }

            INT32 value = 0;
            if (get(expression, value))
            {
                result = (value == 0 ? false : true);
                return true;
            }
            else
            {
                return false;
            }
        }
    }; // namespace Evaluator
}; // namespace Gek
