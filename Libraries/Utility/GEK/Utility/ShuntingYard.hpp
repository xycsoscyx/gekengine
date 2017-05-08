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
			: public WString
		{
		public:
			template<typename TYPE, typename... PARAMETERS>
			BaseString & appendFormat(WString::value_type const *formatting, TYPE const &value, PARAMETERS... arguments)
			{
				auto message(WString::Format(formatting, value, arguments...));
				std::wcout << message << std::endl;
				return WString::append(message);
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
            WString string;
            float value = 0.0f;

            Token(size_t position, TokenType type = TokenType::Unknown);
            Token(size_t position, TokenType type, WString const &string, uint32_t parameterCount = 0);
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
        std::unordered_map<WString, float> variableMap;
        std::unordered_map<WString, Operation> operationsMap;
        std::unordered_map<WString, Function> functionsMap;
        std::mt19937 mersineTwister;
        uint32_t seed = std::mt19937::default_seed;

    public:
        ShuntingYard(void);

        void setRandomSeed(uint32_t seed);
        uint32_t getRandomSeed(void);

        TokenList getTokenList(WString const &expression, WString &logMessage = Logger());
        float evaluate(TokenList &rpnTokenList, float defaultValue, WString &logMessage = Logger());
        float evaluate(WString const &expression, float defaultValue, WString &logMessage = Logger());

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
		bool insertToken(TokenList &infixTokenList, Token &token, WString &logMessage);
        bool parseSubTokens(TokenList &infixTokenList, WString const &token, size_t position, WString &logMessage);
        TokenList convertExpressionToInfix(WString const &expression, WString &logMessage);
        TokenList convertInfixToReversePolishNotation(const TokenList &infixTokenList, WString &logMessage);
        float evaluateReversePolishNotation(const TokenList &rpnTokenList, float defaultValue, WString &logMessage);
    };
}; // namespace Gek
