#pragma once

#include "Gek\Utility\Hash.h"
#include <functional>
#include <unordered_map>
#include <random>
#include <stack>

namespace Gek
{
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

            Token(TokenType type = TokenType::Unknown);
            Token(TokenType type, const CStringW &string, UINT32 parameterCount = 0);
            Token(float value);
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
        ShuntingYard(void);

        Status evaluate(const CStringW &expression, float &value);

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
        bool isNumber(const CStringW &token);
        bool isOperation(const CStringW &token);
        bool isFunction(const CStringW &token);
        bool isLeftParenthesis(const CStringW &token);
        bool isRightParenthesis(const CStringW &token);
        bool isParenthesis(const CStringW &token);
        bool isSeparator(const CStringW &token);
        bool isAssociative(const CStringW &token, const Associations &type);
        int comparePrecedence(const CStringW &token1, const CStringW &token2);
        TokenType getTokenType(const CStringW &token);

    private:
        void insertToken(std::vector<Token> &infixTokenList, Token &token);
        bool replaceFirstVariable(std::vector<Token> &infixTokenList, CStringW &token);
        bool replaceFirstFunction(std::vector<Token> &infixTokenList, CStringW &token);
        Status parseSubTokens(std::vector<Token> &infixTokenList, CStringW token);
        std::vector<Token> convertExpressionToInfix(const CStringW &expression);
        Status convertInfixToReversePolishNotation(const std::vector<Token> &infixTokenList, std::vector<Token> &rpnTokenList);
        Status evaluateReversePolishNotation(const std::vector<Token> &rpnTokenList, float &value);

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
}; // namespace Gek
