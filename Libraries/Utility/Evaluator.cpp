#include "GEK/Utility/Evaluator.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Math/Common.hpp"

namespace Gek
{
    namespace Evaluator
    {
        template <typename TYPE>
        void CastResult(ShuntingYard &shuntingYard, const wchar_t *expression, TYPE &result)
        {
            try
            {
                float value;
                shuntingYard.evaluate(expression, value);
                result = TYPE(value);
            }
            catch (const ShuntingYard::Exception &)
            {
                throw Exception("Unable to evaluate scalar expression");
            };
        }

        template <typename TYPE>
        void GetResult(ShuntingYard &shuntingYard, const wchar_t *expression, TYPE &result)
        {
            try
            {
                ShuntingYard::TokenList rpnTokenList(shuntingYard.getTokenList(expression));
                switch (shuntingYard.getReturnSize(rpnTokenList))
                {
                case 1:
                    if(true)
                    {
                        float value;
                        shuntingYard.evaluate(rpnTokenList, value);
                        result.set(value);
                    }

                    break;

                default:
                    shuntingYard.evaluate(rpnTokenList, result);
                    break;
                };

            }
            catch (const ShuntingYard::Exception &)
            {
                throw Exception("Unable to evaluate vector expression");
            };
        }

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, int32_t &result)
        {
            CastResult(shuntingYard, expression, result);
        }

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, uint32_t &result)
        {
            CastResult(shuntingYard, expression, result);
        }

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, float &result)
        {
            CastResult(shuntingYard, expression, result);
        }

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, Math::Float2 &result)
        {
            GetResult(shuntingYard, expression, result);
        }

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, Math::Float3 &result)
        {
            GetResult(shuntingYard, expression, result);
        }

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, Math::Float4 &result)
        {
            try
            {
                ShuntingYard::TokenList rpnTokenList(shuntingYard.getTokenList(expression));
                switch (shuntingYard.getReturnSize(rpnTokenList))
                {
                case 1:
                    if (true)
                    {
                        float value;
                        shuntingYard.evaluate(expression, value);
                        result.set(value);
                    }

                    break;

                case 3:
                    if (true)
                    {
                        Math::Float3 rgb;
                        shuntingYard.evaluate(expression, rgb);
                        result.set(rgb.x, rgb.y, rgb.z, 1.0f);
                    }

                    break;

                case 4:
                    shuntingYard.evaluate(expression, result);
                    break;

                default:
                    throw Exception("Expression does not evaluate into four component vector");
                };
            }
            catch (const ShuntingYard::Exception &)
            {
                throw Exception("Unable to evaluate vector expression");
            };
        }

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, Math::Quaternion &result)
        {
            try
            {
                ShuntingYard::TokenList rpnTokenList(shuntingYard.getTokenList(expression));
                switch (shuntingYard.getReturnSize(rpnTokenList))
                {
                case 3:
                    if (true)
                    {
                        Math::Float3 euler;
                        shuntingYard.evaluate(expression, euler);
                        result = Math::Quaternion::FromEuler(euler.x, euler.y, euler.z);
                    }

                    break;

                case 4:
                    shuntingYard.evaluate(expression, result);
                    break;

                default:
                    throw Exception("Expression does not evaluate into four component quaternion");
                };
            }
            catch (const ShuntingYard::Exception &)
            {
                throw Exception("Unable to evaluate quaternion expression");
            };
        }

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, String &result)
        {
            result = expression;
        }
    }; // namespace Evaluator
}; // namespace Gek
