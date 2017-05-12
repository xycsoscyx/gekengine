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
		class Logger
			: public std::string
		{
		public:
			template<typename TYPE, typename... PARAMETERS>
            std::string & appendFormat(char const *formatting, TYPE const &value, PARAMETERS... arguments)
			{
				auto message(Format(formatting, value, arguments...));
				std::cout << message << std::endl;
				return append(message);
			}
		};

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
			size_t position = -1;
            TokenType type = TokenType::Unknown;
            uint32_t parameterCount = 0;
            std::string string;
            float value = 0.0f;

            Token(size_t position, TokenType type = TokenType::Unknown);
            Token(size_t position, TokenType type, std::string const &string, uint32_t parameterCount = 0);
			Token(size_t position, float value);
        };

        using TokenList = std::vector<Token>;

    private:
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

    private:
        std::unordered_map<std::string, float> variableMap;
        std::unordered_map<std::string, Operation> operationsMap;
        std::unordered_map<std::string, Function> functionsMap;
        std::mt19937 mersineTwister;
        uint32_t seed = std::mt19937::default_seed;

    public:
        ShuntingYard(void);

        void setRandomSeed(uint32_t seed);
        uint32_t getRandomSeed(void);

        TokenList getTokenList(std::string const &expression, std::string &logMessage = Logger());
        float evaluate(TokenList &rpnTokenList, float defaultValue, std::string &logMessage = Logger());
        float evaluate(std::string const &expression, float defaultValue, std::string &logMessage = Logger());

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
		bool insertToken(TokenList &infixTokenList, Token &token, std::string &logMessage);
        bool parseSubTokens(TokenList &infixTokenList, std::string const &token, size_t position, std::string &logMessage);
        TokenList convertExpressionToInfix(std::string const &expression, std::string &logMessage);
        TokenList convertInfixToReversePolishNotation(const TokenList &infixTokenList, std::string &logMessage);
        float evaluateReversePolishNotation(const TokenList &rpnTokenList, float defaultValue, std::string &logMessage);
    };
}; // namespace Gek
