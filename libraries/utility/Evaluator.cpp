#include "exprtk.hpp"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"
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
                symbolTable.remove_vector("value");
                symbolTable.add_vector("value", vector);
                if (parser.compile(String::format("var vector[%d] := {%S}; value := vector;", SIZE, equation).GetString(), expression))
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

        bool getDouble(LPCWSTR expression, double &result)
        {
            return evaluateDouble.getValue(expression, result);
        }

        bool getFloat(LPCWSTR expression, float &result)
        {
            return evaluateFloat.getValue(expression, result);
        }

        bool getFloat2(LPCWSTR expression, Gek::Math::Float2 &result)
        {
            return evaluateFloat.getVector(expression, result.data);
        }

        bool getFloat3(LPCWSTR expression, Gek::Math::Float3 &result)
        {
            return evaluateFloat.getVector(expression, result.data);
        }

        bool getFloat4(LPCWSTR expression, Gek::Math::Float4 &result)
        {
            return evaluateFloat.getVector(expression, result.data);
        }

        bool getQuaternion(LPCWSTR expression, Gek::Math::Quaternion &result)
        {
            return evaluateFloat.getVector(expression, result.data);
        }

        bool getINT32(LPCWSTR expression, INT32 &result)
        {
            float value = 0.0f;
            if (getFloat(expression, value))
            {
                result = INT32(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool getUINT32(LPCWSTR expression, UINT32 &result)
        {
            float value = 0.0f;
            if (getFloat(expression, value))
            {
                result = UINT32(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool getINT64(LPCWSTR expression, INT64 &result)
        {
            double value = 0.0;
            if (getDouble(expression, value))
            {
                result = INT64(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool getUINT64(LPCWSTR expression, UINT64 &result)
        {
            double value = 0.0;
            if (getDouble(expression, value))
            {
                result = UINT64(value);
                return true;
            }
            else
            {
                return false;
            }
        }

        bool getBoolean(LPCWSTR expression, bool &result)
        {
            if (_wcsicmp(expression, L"true") == 0 ||
                _wcsicmp(expression, L"yes") == 0)
            {
                result = true;
                return true;
            }

            INT32 value = 0;
            if (getINT32(expression, value))
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
