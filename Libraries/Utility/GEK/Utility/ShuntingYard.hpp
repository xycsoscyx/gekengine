/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
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
        GEK_ADD_EXCEPTION(InvalidEquation, Exception);
        GEK_ADD_EXCEPTION(InvalidOperator, Exception);
        GEK_ADD_EXCEPTION(InvalidOperand, Exception);
        GEK_ADD_EXCEPTION(InvalidFunction, Exception);
        GEK_ADD_EXCEPTION(InvalidFunctionParameters, Exception);
        GEK_ADD_EXCEPTION(NotEnoughFunctionParameters, Exception);
        GEK_ADD_EXCEPTION(MissingFunctionParenthesis, Exception);
        GEK_ADD_EXCEPTION(MisplacedSeparator, Exception);

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
        };

        struct Token
        {
            TokenType type;
            uint32_t parameterCount;
            WString string;
            float value;

            Token(TokenType type = TokenType::Unknown);
            Token(TokenType type, WString const &string, uint32_t parameterCount = 0);
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
        std::unordered_map<WString, float> variableMap;
        std::unordered_map<WString, Operation> operationsMap;
        std::unordered_map<WString, Function> functionsMap;
        std::mt19937 mersineTwister;
        uint32_t seed = std::mt19937::default_seed;

    public:
        ShuntingYard(void);

        void setRandomSeed(uint32_t seed);
        uint32_t getRandomSeed(void);

        TokenList getTokenList(WString const &expression);
        float evaluate(TokenList &rpnTokenList);
        float evaluate(WString const &expression);

    private:
        bool isNumber(WString const &token);
        bool isOperation(WString const &token);
        bool isFunction(WString const &token);
        bool isLeftParenthesis(WString const &token);
        bool isRightParenthesis(WString const &token);
        bool isParenthesis(WString const &token);
        bool isSeparator(WString const &token);
        bool isAssociative(WString const &token, const Associations &type);
        int comparePrecedence(WString const &token1, WString const &token2);
        TokenType getTokenType(WString const &token);
        bool isValidReturnType(const Token &token);

    private:
        void insertToken(TokenList &infixTokenList, Token &token);
        void parseSubTokens(TokenList &infixTokenList, WString const &token);
        TokenList convertExpressionToInfix(WString const &expression);
        TokenList convertInfixToReversePolishNotation(const TokenList &infixTokenList);
        float evaluateReversePolishNotation(const TokenList &rpnTokenList);
    };
}; // namespace Gek
