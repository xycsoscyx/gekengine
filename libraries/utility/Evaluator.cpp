#include "GEK\Utility\Evaluator.h"
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
        // https://blog.kallisti.net.nz/2008/02/extension-to-the-shunting-yard-algorithm-to-allow-variable-numbers-of-arguments-to-functions/
        class ShuntingYard
        {
        private:
            enum class Associations : UINT8
            {
                Left = 0,
                Right,
            };

            enum class TokenType : UINT8
            {
                Unknown = 0,
                Number,
                UnaryOperation,
                BinaryOperation,
                LeftParenthesis,
                RightParenthesis,
                Separator,
                Function,
                Vector,
            };

            struct Token
            {
                TokenType type;
                UINT32 parameterCount;
                CStringW string;
                float value;

                Token(TokenType type = TokenType::Unknown)
                    : type(type)
                    , parameterCount(0)
                {
                }

                Token(TokenType type, const CStringW &string, UINT32 parameterCount = 0)
                    : type(type)
                    , string(string)
                    , parameterCount(parameterCount)
                {
                }

                Token(float value)
                    : type(TokenType::Number)
                    , value(value)
                {
                }
            };

            template <typename DATA>
            struct Stack : public std::stack<DATA>
            {
                DATA popTop(void)
                {
                    DATA topElement = top();
                    stack::pop();
                    return topElement;
                }
            };

            struct Operation
            {
                int precedence;
                Associations association;
                std::function<float(float value)> unaryFunction;
                std::function<float(float valueLeft, float valueRight)> binaryFunction;
            };

            struct Function
            {
                UINT32 parameterCount;
                std::function<float(Stack<Token> &)> function;
            };

        private:
            std::unordered_map<CStringW, float> variableMap;
            std::unordered_map<CStringW, Operation> operationsMap;
            std::unordered_map<CStringW, Function> functionsMap;
            std::mt19937 mersineTwister;

        public:
            enum class Status : UINT8
            {
                Success = 0,
                UnknownTokenType,
                UnbalancedParenthesis,
                InvalidVector,
                InvalidEquation,
                InvalidOperator,
                InvalidOperand,
                InvalidFunctionParameters,
                NotEnoughFunctionParameters,
                MissingFunctionParenthesis,
                MisplacedSeparator,
            };

        public:
            ShuntingYard(void)
                : mersineTwister(std::random_device()())
            {
                variableMap[L"pi"] = 3.14159265358979323846;
                variableMap[L"e"] = 2.71828182845904523536;

                operationsMap.insert({ L"^",{ 4, Associations::Right, nullptr, [](float valueLeft, float valueRight) -> float
                {
                    return std::pow(valueLeft, valueRight);
                } } });

                operationsMap.insert({ L"*",{ 3, Associations::Left, nullptr, [](float valueLeft, float valueRight) -> float
                {
                    return (valueLeft * valueRight);
                } } });

                operationsMap.insert({ L"/",{ 3, Associations::Left, nullptr, [](float valueLeft, float valueRight) -> float
                {
                    return (valueLeft / valueRight);
                } } });

                operationsMap.insert({ L"+",{ 2, Associations::Left, [](float value) -> float
                {
                    return value;
                }, [](float valueLeft, float valueRight) -> float
                {
                    return (valueLeft + valueRight);
                } } });

                operationsMap.insert({ L"-",{ 2, Associations::Left, [](float value) -> float
                {
                    return -value;
                }, [](float valueLeft, float valueRight) -> float
                {
                    return (valueLeft - valueRight);
                } } });

                functionsMap.insert({ L"sin",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::sin(value);
                } } });

                functionsMap.insert({ L"cos",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::cos(value);
                } } });

                functionsMap.insert({ L"tan",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::tan(value);
                } } });

                functionsMap.insert({ L"asin",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::asin(value);
                } } });

                functionsMap.insert({ L"acos",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::acos(value);
                } } });

                functionsMap.insert({ L"atan",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::atan(value);
                } } });

                functionsMap.insert({ L"min",{ 2, [](Stack<Token> &stack) -> float
                {
                    float value2 = stack.popTop().value;
                    float value1 = stack.popTop().value;
                    return std::min(value1, value2);
                } } });

                functionsMap.insert({ L"max",{ 2, [](Stack<Token> &stack) -> float
                {
                    float value2 = stack.popTop().value;
                    float value1 = stack.popTop().value;
                    return std::max(value1, value2);
                } } });

                functionsMap.insert({ L"abs",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::abs(value);
                } } });

                functionsMap.insert({ L"ceil",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::ceil(value);
                } } });

                functionsMap.insert({ L"floor",{ 1, [](Stack<Token> &stack) -> float
                {
                    float value = stack.popTop().value;
                    return std::floor(value);
                } } });

                functionsMap.insert({ L"lerp",{ 3, [](Stack<Token> &stack) -> float
                {
                    float value3 = stack.popTop().value;
                    float value2 = stack.popTop().value;
                    float value1 = stack.popTop().value;
                    return Math::lerp(value1, value2, value3);
                } } });

                functionsMap.insert({ L"random",{ 2, [&](Stack<Token> &stack) -> float
                {
                    static std::random_device randomDevice;
                    static std::mt19937 mersineTwister(randomDevice());

                    float value2 = stack.popTop().value;
                    float value1 = stack.popTop().value;
                    std::uniform_real_distribution<float> uniformRealDistribution(value1, value2);
                    return uniformRealDistribution(mersineTwister);
                } } });
            }

            Status evaluate(const CStringW &expression, float &value)
            {
                std::vector<Token> infixTokenList = convertExpressionToInfix(expression);

                std::vector<Token> rpnTokenList;
                Status status = convertInfixToReversePolishNotation(infixTokenList, rpnTokenList);
                if (status == Status::Success)
                {
                    status = evaluateReversePolishNotation(rpnTokenList, value);
                }

                return status;
            }

            template <typename TYPE>
            Status evaluate(const CStringW &expression, TYPE &value)
            {
                std::vector<Token> infixTokenList = convertExpressionToInfix(expression);

                std::vector<Token> rpnTokenList;
                Status status = convertInfixToReversePolishNotation(infixTokenList, rpnTokenList);
                if (status == Status::Success)
                {
                    status = evaluateReversePolishNotation(rpnTokenList, value.data);
                }

                return status;
            }

        private:
            bool isNumber(const CStringW &token)
            {
                return std::regex_match(token.GetString(), std::wregex(L"^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$"));
            }

            bool isOperation(const CStringW &token)
            {
                return (operationsMap.count(token) > 0);
            }

            bool isFunction(const CStringW &token)
            {
                return (functionsMap.count(token) > 0);
            }

            bool isLeftParenthesis(const CStringW &token)
            {
                return token == L'(';
            }

            bool isRightParenthesis(const CStringW &token)
            {
                return token == L')';
            }

            bool isParenthesis(const CStringW &token)
            {
                return (isLeftParenthesis(token) || isRightParenthesis(token));
            }

            bool isSeparator(const CStringW &token)
            {
                return token == L',';
            }

            bool isAssociative(const CStringW &token, const Associations &type)
            {
                auto &p = operationsMap.find(token)->second;
                return p.association == type;
            }

            int comparePrecedence(const CStringW &token1, const CStringW &token2)
            {
                auto &p1 = operationsMap.find(token1)->second;
                auto &p2 = operationsMap.find(token2)->second;
                return p1.precedence - p2.precedence;
            }

            TokenType getTokenType(const CStringW &token)
            {
                if (isNumber(token))
                {
                    return TokenType::Number;
                }
                else if (isOperation(token))
                {
                    return TokenType::BinaryOperation;
                }
                else if (isFunction(token))
                {
                    return TokenType::Function;
                }
                else if (isSeparator(token))
                {
                    return TokenType::Separator;
                }
                else if (isLeftParenthesis(token))
                {
                    return TokenType::LeftParenthesis;
                }
                else if (isRightParenthesis(token))
                {
                    return TokenType::RightParenthesis;
                }

                return TokenType::Unknown;
            }

        private:
            void insertToken(std::vector<Token> &infixTokenList, Token &token)
            {
                if (!infixTokenList.empty())
                {
                    const Token &previous = infixTokenList.back();

                    // ) 2 or 2 2
                    if (token.type == TokenType::Number && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
                    {
                        infixTokenList.push_back(Token(TokenType::BinaryOperation, L"*"));
                    }
                    // 2 ( or ) (
                    else if (token.type == TokenType::LeftParenthesis && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
                    {
                        infixTokenList.push_back(Token(TokenType::BinaryOperation, L"*"));
                    }
                    // ) sin or 2 sin
                    else if (token.type == TokenType::Function && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
                    {
                        infixTokenList.push_back(Token(TokenType::BinaryOperation, L"*"));
                    }
                }

                if (token.type == TokenType::BinaryOperation)
                {
                    if (token.string == L"-" || token.string == L"+")
                    {
                        // -3 or -sin
                        if (infixTokenList.empty())
                        {
                            token.type = TokenType::UnaryOperation;
                        }
                        else
                        {
                            const Token &previous = infixTokenList.back();

                            // 2+-3 or sin(1)*-1
                            if (previous.type == TokenType::BinaryOperation || previous.type == TokenType::UnaryOperation)
                            {
                                token.type = TokenType::UnaryOperation;
                            }
                            // (-3)
                            else if (previous.type == TokenType::LeftParenthesis)
                            {
                                token.type = TokenType::UnaryOperation;
                            }
                            // ,-3
                            else if (previous.type == TokenType::Separator)
                            {
                                token.type = TokenType::UnaryOperation;
                            }
                        }
                    }
                }

                infixTokenList.push_back(token);
            }

            bool replaceFirstVariable(std::vector<Token> &infixTokenList, CStringW &token)
            {
                for (auto &variable : variableMap)
                {
                    if (token.Find(variable.first) == 0)
                    {
                        insertToken(infixTokenList, Token(variable.second));
                        token = token.Mid(variable.first.GetLength());
                        return true;
                    }
                }

                return false;
            }

            bool replaceFirstFunction(std::vector<Token> &infixTokenList, CStringW &token)
            {
                for (auto &function : functionsMap)
                {
                    if (token.Find(function.first) == 0)
                    {
                        insertToken(infixTokenList, Token(TokenType::Function, function.first));
                        token = token.Mid(function.first.GetLength());
                        return true;
                    }
                }

                return false;
            }

            Status parseSubTokens(std::vector<Token> &infixTokenList, CStringW token)
            {
                while (!token.IsEmpty())
                {
                    std::wcmatch matches;
                    if (std::regex_search(token.GetString(), matches, std::wregex(L"^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?")))
                    {
                        CStringW value = matches[0].str().c_str();
                        insertToken(infixTokenList, Token(Gek::String::to<float>(value)));
                        token = token.Mid(value.GetLength());
                        continue;
                    }

                    if (replaceFirstVariable(infixTokenList, token))
                    {
                        continue;
                    }

                    if (replaceFirstFunction(infixTokenList, token))
                    {
                        if (!token.IsEmpty())
                        {
                            // function must be followed by a left parenthesis, so it has to be at the end of an implicit block
                            return Status::MissingFunctionParenthesis;
                        }

                        continue;
                    }

                    if (!token.IsEmpty())
                    {
                        // nothing was replaced, yet we still have remaining characters?
                        return Status::UnknownTokenType;
                    }
                };

                return Status::Success;
            }

            std::vector<Token> convertExpressionToInfix(const CStringW &expression)
            {
                CStringW runningToken;
                std::vector<Token> infixTokenList;
                for (int index = 0; index < expression.GetLength(); ++index)
                {
                    CStringW nextToken = expression.GetAt(index);
                    if (isOperation(nextToken) || isParenthesis(nextToken) || isSeparator(nextToken))
                    {
                        if (!runningToken.IsEmpty())
                        {
                            parseSubTokens(infixTokenList, runningToken);
                            runningToken.Empty();
                        }

                        insertToken(infixTokenList, Token(getTokenType(nextToken), nextToken));
                    }
                    else
                    {
                        if (nextToken == L" ")
                        {
                            if (!runningToken.IsEmpty())
                            {
                                parseSubTokens(infixTokenList, runningToken);
                                runningToken.Empty();
                            }
                        }
                        else
                        {
                            runningToken.Append(nextToken);
                        }
                    }
                }

                if (!runningToken.IsEmpty())
                {
                    parseSubTokens(infixTokenList, runningToken);
                }

                return infixTokenList;
            }

            Status convertInfixToReversePolishNotation(const std::vector<Token> &infixTokenList, std::vector<Token> &rpnTokenList)
            {
                Stack<Token> stack;
                Stack<bool> parameterExistsStack;
                Stack<UINT32> parameterCountStack;
                for (auto &token : infixTokenList)
                {
                    switch (token.type)
                    {
                    case TokenType::Number:
                        rpnTokenList.push_back(token);
                        if (!parameterExistsStack.empty())
                        {
                            parameterExistsStack.pop();
                            parameterExistsStack.push(true);
                        }

                        break;

                    case TokenType::UnaryOperation:
                        stack.push(token);
                        break;

                    case TokenType::BinaryOperation:
                        while (!stack.empty() && (stack.top().type == TokenType::BinaryOperation &&
                            (isAssociative(token.string, Associations::Left) && comparePrecedence(token.string, stack.top().string) == 0) ||
                            (isAssociative(token.string, Associations::Right) && comparePrecedence(token.string, stack.top().string) < 0)))
                        {
                            rpnTokenList.push_back(stack.popTop());
                        };

                        stack.push(token);
                        break;

                    case TokenType::LeftParenthesis:
                        // only return vector as a final value
                        if (stack.empty() || (stack.top().type == TokenType::Function))
                        {
                            parameterCountStack.push(0);
                            if (!parameterExistsStack.empty())
                            {
                                parameterExistsStack.pop();
                                parameterExistsStack.push(true);
                            }

                            parameterExistsStack.push(false);
                        }

                        stack.push(token);
                        break;

                    case TokenType::RightParenthesis:
                        if (stack.empty())
                        {
                            // we can't have an ending parenthesis without a starting one
                            return Status::UnbalancedParenthesis;
                        }

                        while (stack.top().type != TokenType::LeftParenthesis)
                        {
                            rpnTokenList.push_back(stack.popTop());
                            if (stack.empty())
                            {
                                // found a closing parenthesis without a starting one
                                return Status::UnbalancedParenthesis;
                            }
                        };

                        stack.pop();
                        if (stack.empty())
                        {
                            Token vector(TokenType::Vector);
                            vector.parameterCount = parameterCountStack.popTop();
                            if (parameterExistsStack.popTop())
                            {
                                vector.parameterCount++;
                            }

                            // don't push single value ()'s as vectors
                            if (vector.parameterCount > 1)
                            {
                                rpnTokenList.push_back(vector);
                            }
                        }
                        else if (stack.top().type == TokenType::Function)
                        {
                            Token function = stack.popTop();
                            function.parameterCount = parameterCountStack.popTop();
                            if (parameterExistsStack.popTop())
                            {
                                function.parameterCount++;
                            }

                            rpnTokenList.push_back(function);
                        }

                        break;

                    case TokenType::Separator:
                        if (stack.empty() || parameterExistsStack.empty())
                        {
                            // we can't not have values on the stack and find a parameter separator
                            return Status::MisplacedSeparator;
                        }

                        while (stack.top().type != TokenType::LeftParenthesis)
                        {
                            rpnTokenList.push_back(stack.popTop());
                            if (stack.empty())
                            {
                                // found a separator without a starting parenthesis
                                return Status::MisplacedSeparator;
                            }
                        };

                        if (parameterExistsStack.top())
                        {
                            parameterCountStack.top()++;
                        }

                        parameterExistsStack.pop();
                        parameterExistsStack.push(false);
                        break;

                    case TokenType::Function:
                        stack.push(token);
                        break;

                    default:
                        return Status::UnknownTokenType;
                    };
                }

                while (!stack.empty())
                {
                    if (stack.top().type == TokenType::LeftParenthesis ||
                        stack.top().type == TokenType::RightParenthesis)
                    {
                        // all parenthesis should have been handled above
                        return Status::UnbalancedParenthesis;
                    }

                    rpnTokenList.push_back(stack.popTop());
                };

                return Status::Success;
            }

            Status evaluateReversePolishNotation(const std::vector<Token> &rpnTokenList, float &value)
            {
                Stack<Token> stack;
                for (auto &token : rpnTokenList)
                {
                    switch (token.type)
                    {
                    case TokenType::Number:
                        stack.push(token);
                        break;

                    case TokenType::UnaryOperation:
                        if (true)
                        {
                            auto &operation = operationsMap.find(token.string)->second;
                            if (operation.unaryFunction)
                            {
                                if (!stack.empty() && (stack.top().type == TokenType::Number))
                                {
                                    float functionValue = stack.popTop().value;
                                    stack.push(Token(operation.unaryFunction(functionValue)));
                                }
                                else
                                {
                                    return Status::InvalidOperand;
                                }
                            }
                            else
                            {
                                return Status::InvalidOperator;
                            }

                            break;
                        }

                    case TokenType::BinaryOperation:
                        if (true)
                        {
                            auto &operation = operationsMap.find(token.string)->second;
                            if (operation.binaryFunction)
                            {
                                if (!stack.empty() && (stack.top().type == TokenType::Number))
                                {
                                    float functionValueRight = stack.popTop().value;
                                    if (!stack.empty() && (stack.top().type == TokenType::Number))
                                    {
                                        float functionValueLeft = stack.popTop().value;
                                        stack.push(Token(operation.binaryFunction(functionValueLeft, functionValueRight)));
                                    }
                                    else
                                    {
                                        return Status::InvalidOperand;
                                    }
                                }
                                else
                                {
                                    return Status::InvalidOperand;
                                }
                            }
                            else
                            {
                                return Status::InvalidOperator;
                            }

                            break;
                        }

                    case TokenType::Function:
                        if (true)
                        {
                            auto &function = functionsMap.find(token.string)->second;
                            if (function.parameterCount != token.parameterCount)
                            {
                                return Status::InvalidFunctionParameters;
                            }

                            if (stack.size() < function.parameterCount)
                            {
                                return Status::NotEnoughFunctionParameters;
                            }

                            stack.push(Token(function.function(stack)));
                            break;
                        }

                    case TokenType::Vector:
                        return Status::InvalidEquation;

                    default:
                        return Status::UnknownTokenType;
                    };
                }

                if (stack.size() == 1)
                {
                    value = stack.top().value;
                    return Status::Success;
                }
                else
                {
                    return Status::InvalidEquation;
                }
            }

            template <std::size_t SIZE>
            Status evaluateReversePolishNotation(const std::vector<Token> &rpnTokenList, float(&value)[SIZE])
            {
                Stack<Token> stack;
                for (auto &token : rpnTokenList)
                {
                    switch (token.type)
                    {
                    case TokenType::Number:
                        stack.push(token);
                        break;

                    case TokenType::UnaryOperation:
                        if (true)
                        {
                            auto &operation = operationsMap.find(token.string)->second;
                            if (operation.unaryFunction)
                            {
                                if (!stack.empty() && (stack.top().type == TokenType::Number))
                                {
                                    float functionValue = stack.popTop().value;
                                    stack.push(Token(operation.unaryFunction(functionValue)));
                                }
                                else
                                {
                                    return Status::InvalidOperand;
                                }
                            }
                            else
                            {
                                return Status::InvalidOperator;
                            }

                            break;
                        }

                    case TokenType::BinaryOperation:
                        if (true)
                        {
                            auto &operation = operationsMap.find(token.string)->second;
                            if (operation.binaryFunction)
                            {
                                if (!stack.empty() && (stack.top().type == TokenType::Number))
                                {
                                    float functionValueRight = stack.popTop().value;
                                    if (!stack.empty() && (stack.top().type == TokenType::Number))
                                    {
                                        float functionValueLeft = stack.popTop().value;
                                        stack.push(Token(operation.binaryFunction(functionValueLeft, functionValueRight)));
                                    }
                                    else
                                    {
                                        return Status::InvalidOperand;
                                    }
                                }
                                else
                                {
                                    return Status::InvalidOperand;
                                }
                            }
                            else
                            {
                                return Status::InvalidOperator;
                            }

                            break;
                        }

                    case TokenType::Function:
                        if (true)
                        {
                            auto &function = functionsMap.find(token.string)->second;
                            if (function.parameterCount != token.parameterCount)
                            {
                                return Status::InvalidFunctionParameters;
                            }

                            if (stack.size() < function.parameterCount)
                            {
                                return Status::NotEnoughFunctionParameters;
                            }

                            stack.push(Token(function.function(stack)));
                            break;
                        }

                    case TokenType::Vector:
                        if (token.parameterCount != SIZE)
                        {
                            return Status::InvalidVector;
                        }

                        if (stack.size() != SIZE)
                        {
                            return Status::InvalidVector;
                        }

                        for (UINT32 axis = SIZE; axis > 0; axis--)
                        {
                            value[axis - 1] = stack.popTop().value;
                        }

                        return Status::Success;

                    default:
                        return Status::UnknownTokenType;
                    };
                }

                if (SIZE == 1 && stack.size() == 1)
                {
                    value[0] = stack.top().value;
                    return Status::Success;
                }
                else
                {
                    return Status::InvalidEquation;
                }
            }
        };

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
