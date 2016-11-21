/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include <unordered_map>
#include <functional>
#include <random>
#include <stack>

namespace Gek
{
    class ShuntingYard
    {
    public:
        GEK_ADD_EXCEPTION();
        GEK_ADD_EXCEPTION(UnknownTokenType, Exception);
        GEK_ADD_EXCEPTION(UnbalancedParenthesis, Exception);
        GEK_ADD_EXCEPTION(InvalidReturnType, Exception);
        GEK_ADD_EXCEPTION(InvalidVector, Exception);
        GEK_ADD_EXCEPTION(InvalidEquation, Exception);
        GEK_ADD_EXCEPTION(InvalidOperator, Exception);
        GEK_ADD_EXCEPTION(InvalidOperand, Exception);
        GEK_ADD_EXCEPTION(InvalidFunction, Exception);
        GEK_ADD_EXCEPTION(InvalidFunctionParameters, Exception);
        GEK_ADD_EXCEPTION(NotEnoughFunctionParameters, Exception);
        GEK_ADD_EXCEPTION(MissingFunctionParenthesis, Exception);
        GEK_ADD_EXCEPTION(MisplacedSeparator, Exception);
        GEK_ADD_EXCEPTION(VectorUsedAsParameter, Exception);

    public:
        enum class Associations : uint8_t
        {
            Left = 0,
            Right,
        };

        enum class TokenType : uint8_t
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
            uint32_t parameterCount;
            String string;
            float value;

            Token(TokenType type = TokenType::Unknown);
            Token(TokenType type, const wchar_t *string, uint32_t parameterCount = 0);
            Token(float value);
        };

        using TokenList = std::vector<Token>;

    private:
        template <typename DATA>
        struct Stack
            : public std::stack<DATA>
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
            uint32_t parameterCount;
            std::function<float(Stack<Token> &)> function;
        };

    private:
        std::unordered_map<String, float> variableMap;
        std::unordered_map<String, Operation> operationsMap;
        std::unordered_map<String, Function> functionsMap;
        std::mt19937 mersineTwister;

    public:
        ShuntingYard(void);

        void setRandomSeed(uint32_t seed);

        TokenList getTokenList(const wchar_t *expression);
        uint32_t getReturnSize(const TokenList &rpnTokenList);

        inline void evaluate(TokenList &rpnTokenList, float &value)
        {
            evaluateValue(rpnTokenList, &value, 1);
        }

        template <std::size_t SIZE>
        void evaluate(TokenList &rpnTokenList, float(&value)[SIZE])
        {
            evaluateValue(rpnTokenList, value, SIZE);
        }

        template <class TYPE>
        void evaluate(TokenList &rpnTokenList, TYPE &value)
        {
            evaluate(rpnTokenList, value.data);
        }

        inline void evaluate(const wchar_t *expression, float &value)
        {
            evaluateValue(expression, &value, 1);
        }

        template <std::size_t SIZE>
        void evaluate(const wchar_t *expression, float(&value)[SIZE])
        {
            evaluateValue(expression, value, SIZE);
        }

        template <class TYPE>
        void evaluate(const wchar_t *expression, TYPE &value)
        {
            evaluate(expression, value.data);
        }

    private:
        bool isNumber(const wchar_t *token);
        bool isOperation(const wchar_t *token);
        bool isFunction(const wchar_t *token);
        bool isLeftParenthesis(const wchar_t *token);
        bool isRightParenthesis(const wchar_t *token);
        bool isParenthesis(const wchar_t *token);
        bool isSeparator(const wchar_t *token);
        bool isAssociative(const wchar_t *token, const Associations &type);
        int comparePrecedence(const wchar_t *token1, const wchar_t *token2);
        TokenType getTokenType(const wchar_t *token);
        bool isValidReturnType(const Token &token);

    private:
        void insertToken(TokenList &infixTokenList, Token &token);
        void parseSubTokens(TokenList &infixTokenList, const String &token);
        TokenList convertExpressionToInfix(const String &expression);
        TokenList convertInfixToReversePolishNotation(const TokenList &infixTokenList);
        void evaluateReversePolishNotation(const TokenList &rpnTokenList, float *value, uint32_t valueSize);

        template <std::size_t SIZE>
        void evaluateReversePolishNotation(const TokenList &rpnTokenList, float(&value)[SIZE])
        {
            evaluateReversePolishNotation(rpnTokenList, value, SIZE);
        }

        void evaluateValue(TokenList &rpnTokenList, float *value, uint32_t valueSize);
        void evaluateValue(const wchar_t *expression, float *value, uint32_t valueSize);
    };
}; // namespace Gek
