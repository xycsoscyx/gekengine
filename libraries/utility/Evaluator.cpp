#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\ShuntingYard.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"
#include <unordered_map>
#include <functional>
#include <random>
#include <stack>

namespace Gek
{
    namespace Evaluator
    {
        static ShuntingYard shuntingYard;

        template <typename TYPE>
        void castResult(const wstring &expression, TYPE &result, TYPE defaultValue)
        {
            try
            {
                float value;
                shuntingYard.evaluate(expression, value);
                result = TYPE(value);
            }
            catch (const ShuntingYard::Exception &exception)
            {
                result = defaultValue;
            };
        }

        template <typename TYPE>
        void getResult(const wstring &expression, TYPE &result, const TYPE &defaultValue)
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
                        shuntingYard.evaluate(expression, value);
                        result.set(value);
                    }

                    break;

                default:
                    shuntingYard.evaluate(expression, result);
                    break;
                };

            }
            catch (const ShuntingYard::Exception &exception)
            {
                result = defaultValue;
            };
        }

        void get(const wstring &expression, INT32 &result, INT32 defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wstring &expression, UINT32 &result, UINT32 defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wstring &expression, INT64 &result, INT64 defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wstring &expression, UINT64 &result, UINT64 defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wstring &expression, float &result, float defaultValue)
        {
            castResult(expression, result, defaultValue);
        }

        void get(const wstring &expression, Math::Float2 &result, const Math::Float2 &defaultValue)
        {
            getResult(expression, result, defaultValue);
        }

        void get(const wstring &expression, Math::Float3 &result, const Math::Float3 &defaultValue)
        {
            getResult(expression, result, defaultValue);
        }

        void get(const wstring &expression, Math::Float4 &result, const Math::Float4 &defaultValue)
        {
            getResult(expression, result, defaultValue);
        }

        void get(const wstring &expression, Math::Color &result, const Math::Color &defaultValue)
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
                        Math::Float3 rgbValue;
                        shuntingYard.evaluate(expression, rgbValue);
                        result.set(rgbValue);
                    }

                    break;

                default:
                    shuntingYard.evaluate(expression, result);
                    break;
                };
            }
            catch (const ShuntingYard::Exception &exception)
            {
                result = defaultValue;
            };
        }

        void get(const wstring &expression, Math::Quaternion &result, const Math::Quaternion &defaultValue)
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
                        result.setEulerRotation(euler.x, euler.y, euler.z);
                    }

                    break;

                default:
                    shuntingYard.evaluate(expression, result);
                    break;
                };
            }
            catch (const ShuntingYard::Exception &exception)
            {
                result = defaultValue;
            };
        }

        void get(const wstring &expression, wstring &result)
        {
            result = expression;
        }
    }; // namespace Evaluator
}; // namespace Gek
