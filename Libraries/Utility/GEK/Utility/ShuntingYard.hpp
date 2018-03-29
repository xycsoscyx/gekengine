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
#include <optional>
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
            Variable,
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
            float *variable = nullptr;

            Token(TokenType type = TokenType::Unknown);
            Token(TokenType type, std::string const &string, uint32_t parameterCount = 0);
            Token(float value);
            Token(float *variable);
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
                float *variable;
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
        ShuntingYard(ShuntingYard const &shuntingYard);

        void setVariable(std::string const &name, float value);
        void setOperation(std::string const &name, int precedence, Associations association, std::function<float(float value)> &unaryFunction, std::function<float(float valueLeft, float valueRight)> &binaryFunction);
        void setFunction(std::string const &name, uint32_t parameterCount, std::function<float(std::stack<float> &)> &function);

        void setRandomSeed(uint32_t seed);
        uint32_t getRandomSeed(void);

        std::optional<OperandList> getTokenList(std::string const &expression);
        std::optional<float> evaluate(OperandList &rpnOperandList);
        std::optional<float> evaluate(std::string const &expression);

    private:
        bool isAssociative(std::string const &token, const Associations &type);
        int comparePrecedence(std::string const &token1, std::string const &token2);

    private:
        Operand getOperand(Token const &token);
		bool insertToken(TokenList &infixTokenList, Token &token);
        std::optional<TokenList> convertExpressionToInfix(std::string const &expression);
        std::optional<OperandList> convertInfixToReversePolishNotation(TokenList const &infixTokenList);
        std::optional<float> evaluateReversePolishNotation(OperandList const &rpnOperandList);
    };
}; // namespace Gek
