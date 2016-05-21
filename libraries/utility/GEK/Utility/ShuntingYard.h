#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\Hash.h"
#include <functional>
#include <unordered_map>
#include <random>
#include <stack>

namespace Gek
{
    class ShuntingYard
    {
    public:
        GEK_BASE_EXCEPTION();
        GEK_EXCEPTION(UnknownTokenType);
        GEK_EXCEPTION(UnbalancedParenthesis);
        GEK_EXCEPTION(InvalidVector);
        GEK_EXCEPTION(InvalidEquation);
        GEK_EXCEPTION(InvalidOperator);
        GEK_EXCEPTION(InvalidOperand);
        GEK_EXCEPTION(InvalidFunction);
        GEK_EXCEPTION(InvalidFunctionParameters);
        GEK_EXCEPTION(NotEnoughFunctionParameters);
        GEK_EXCEPTION(MissingFunctionParenthesis);
        GEK_EXCEPTION(MisplacedSeparator);

    public:
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

    private:
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
        ShuntingYard(void);

        void evaluteTokenList(LPCWSTR expression, std::vector<Token> &rpnTokenList);

        inline void evaluate(std::vector<Token> &rpnTokenList, float &value)
        {
            evaluateValue(rpnTokenList, &value, 1);
        }

        template <std::size_t SIZE>
        void evaluate(std::vector<Token> &rpnTokenList, float(&value)[SIZE])
        {
            evaluateValue(rpnTokenList, value, SIZE);
        }

        template <class TYPE>
        void evaluate(std::vector<Token> &rpnTokenList, TYPE &value)
        {
            evaluate(rpnTokenList, value.data);
        }

        inline void evaluate(LPCWSTR expression, float &value)
        {
            evaluateValue(expression, &value, 1);
        }

        template <std::size_t SIZE>
        void evaluate(LPCWSTR expression, float(&value)[SIZE])
        {
            evaluateValue(expression, value, SIZE);
        }

        template <class TYPE>
        void evaluate(LPCWSTR expression, TYPE &value)
        {
            evaluate(expression, value.data);
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
        void parseSubTokens(std::vector<Token> &infixTokenList, CStringW token);
        std::vector<Token> convertExpressionToInfix(const CStringW &expression);
        void convertInfixToReversePolishNotation(const std::vector<Token> &infixTokenList, std::vector<Token> &rpnTokenList);
        void evaluateReversePolishNotation(const std::vector<Token> &rpnTokenList, float *value, UINT32 valueSize);

        template <std::size_t SIZE>
        void evaluateReversePolishNotation(const std::vector<Token> &rpnTokenList, float(&value)[SIZE])
        {
            evaluateReversePolishNotation(rpnTokenList, value, SIZE);
        }

        void evaluateValue(std::vector<Token> &rpnTokenList, float *value, UINT32 valueSize);
        void evaluateValue(LPCWSTR expression, float *value, UINT32 valueSize);
    };
}; // namespace Gek
