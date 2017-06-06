/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/String.hpp"
#include <unordered_map>
#include <functional>
#include <iostream>
#include <random>
#include <stack>

namespace Gek
{
    class ShuntingYard
    {
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

        enum class OperandType : uint8_t
        {
            Unknown = 0,
            Number,
            UnaryOperation,
            BinaryOperation,
            Function,
        };

        struct Token
        {
            TokenType type = TokenType::Unknown;
            uint32_t parameterCount = 0;
            std::string string;
            float value = 0.0f;

            Token(TokenType type = TokenType::Unknown);
            Token(TokenType type, std::string const &string, uint32_t parameterCount = 0);
			Token(float value);
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
            std::function<float(std::stack<float> &)> function;
        };

        struct Operand
        {
            OperandType type = OperandType::Unknown;
            union
            {
                float value;
                Operation *operation;
                Function *function;
            };

            Operand(void);
            Operand(Token const &token);
            Operand(Token const &token, Operation *operation);
            Operand(Token const &token, Function *function);

        };

        using TokenList = std::vector<Token>;
        using OperandList = std::vector<Operand>;

    private:
        uint32_t seed = std::mt19937::default_seed;
        std::unordered_map<std::string, float> variableMap;
        std::unordered_map<std::string, Operation> operationsMap;
        std::unordered_map<std::string, Function> functionsMap;
        std::mt19937 mersineTwister;

        std::unordered_map<size_t, OperandList> cache;

    public:
        ShuntingYard(void);

        void setRandomSeed(uint32_t seed);
        uint32_t getRandomSeed(void);

        OperandList getTokenList(std::string const &expression);
        float evaluate(OperandList &rpnOperandList, float defaultValue);
        float evaluate(std::string const &expression, float defaultValue);

    private:
        bool isNumber(std::string const &token);
        bool isOperation(std::string const &token);
        bool isFunction(std::string const &token);
        bool isLeftParenthesis(std::string const &token);
        bool isRightParenthesis(std::string const &token);
        bool isParenthesis(std::string const &token);
        bool isSeparator(std::string const &token);
        bool isAssociative(std::string const &token, const Associations &type);
        int comparePrecedence(std::string const &token1, std::string const &token2);
        TokenType getTokenType(std::string const &token);
        bool isValidReturnType(const Token &token);

    private:
        Operand getOperand(Token const &token);
		bool insertToken(TokenList &infixTokenList, Token &token);
        bool parseSubTokens(TokenList &infixTokenList, std::string const &token, size_t position);
        TokenList convertExpressionToInfix(std::string const &expression);
        OperandList convertInfixToReversePolishNotation(const TokenList &infixTokenList);
        float evaluateReversePolishNotation(const OperandList &rpnOperandList, float defaultValue);
    };
}; // namespace Gek
