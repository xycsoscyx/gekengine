#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Math/Common.hpp"
#include <regex>

// https://blog.kallisti.net.rz.xyz/2008/02/extension-to-the-shunting-yard-algorithm-to-allow-variable-numbers-of-arguments-to-functions/

namespace Gek
{
    #define SEARCH_NUMBER "([+-]?(?:(?:\\d+(?:\\.\\d*)?)|(?:\\.\\d+))(?:e\\d+)?)"
    static const std::regex SearchNumber(SEARCH_NUMBER, std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
    static const std::regex SearchWord("([a-z]+)|" SEARCH_NUMBER, std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
	static const ShuntingYard::TokenList EmptyTokenList;

	template <typename TYPE>
	TYPE PopTop(std::stack<TYPE> &stack)
	{
		auto top = stack.top();
		stack.pop();
		return top;
	}

	ShuntingYard::Token::Token(size_t position, ShuntingYard::TokenType type)
		: position(position)
        , type(type)
    {
    }

    ShuntingYard::Token::Token(size_t position, ShuntingYard::TokenType type, std::string const &string, uint32_t parameterCount)
		: position(position)
		, type(type)
        , string(string)
        , parameterCount(parameterCount)
    {
    }

	ShuntingYard::Token::Token(size_t position, float value)
		: position(position)
		, type(TokenType::Number)
		, value(value)
	{
	}

	ShuntingYard::ShuntingYard(void)
        : mersineTwister(std::random_device()())
    {
        variableMap["pi"] = Math::Pi;
        variableMap["tau"] = Math::Tau;
        variableMap["e"] = Math::E;
        variableMap["true"] = 1.0f;
        variableMap["false"] = 0.0f;

        operationsMap.insert({ "^", { 4, Associations::Right, nullptr, [](float valueLeft, float valueRight) -> float
        {
            return std::pow(valueLeft, valueRight);
        } } });

        operationsMap.insert({ "*", { 3, Associations::Left, nullptr, [](float valueLeft, float valueRight) -> float
        {
            return (valueLeft * valueRight);
        } } });

        operationsMap.insert({ "/", { 3, Associations::Left, nullptr, [](float valueLeft, float valueRight) -> float
        {
            return (valueLeft / valueRight);
        } } });

        operationsMap.insert({ "+", { 2, Associations::Left, [](float value) -> float
        {
            return value;
        }, [](float valueLeft, float valueRight) -> float
        {
            return (valueLeft + valueRight);
        } } });

        operationsMap.insert({ "-", { 2, Associations::Left, [](float value) -> float
        {
            return -value;
        }, [](float valueLeft, float valueRight) -> float
        {
            return (valueLeft - valueRight);
        } } });

        functionsMap.insert({ "sin", { 1, [](std::stack<float> &stack) -> float
        {
            float value = stack.top();
            return std::sin(value);
        } } });

        functionsMap.insert({ "cos", { 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::cos(value);
        } } });

        functionsMap.insert({ "tan", { 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::tan(value);
        } } });

        functionsMap.insert({ "asin", { 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::asin(value);
        } } });

        functionsMap.insert({ "acos", { 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::acos(value);
        } } });

        functionsMap.insert({ "atan", { 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::atan(value);
        } } });

        functionsMap.insert({ "min", { 2, [](std::stack<float> &stack) -> float
        {
            float value2 = PopTop(stack);
            float value1 = PopTop(stack);
            return std::min(value1, value2);
        } } });

        functionsMap.insert({ "max", { 2, [](std::stack<float> &stack) -> float
        {
            float value2 = PopTop(stack);
            float value1 = PopTop(stack);
            return std::max(value1, value2);
        } } });

        functionsMap.insert({ "abs", { 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::abs(value);
        } } });

        functionsMap.insert({ "ceil", { 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::ceil(value);
        } } });

        functionsMap.insert({ "floor", { 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::floor(value);
        } } });

        functionsMap.insert({ "lerp", { 3, [](std::stack<float> &stack) -> float
        {
            float value3 = PopTop(stack);
            float value2 = PopTop(stack);
            float value1 = PopTop(stack);
            return Math::Interpolate(value1, value2, value3);
        } } });

        functionsMap.insert({ "random", { 2, [&](std::stack<float> &stack) -> float
        {
            float value2 = PopTop(stack);
            float value1 = PopTop(stack);
            std::uniform_real_distribution<float> uniformRealDistribution(value1, value2);
            return uniformRealDistribution(mersineTwister);
        } } });
    }

    void ShuntingYard::setRandomSeed(uint32_t seed)
    {
        this->seed = seed;
        mersineTwister.seed(seed);
    }

    uint32_t ShuntingYard::getRandomSeed()
    {
        return seed;
    }

    ShuntingYard::TokenList ShuntingYard::getTokenList(std::string const &expression, std::string &logMessage)
    {
		TokenList infixTokenList(convertExpressionToInfix(expression, logMessage));
        return convertInfixToReversePolishNotation(infixTokenList, logMessage);
    }

    float ShuntingYard::evaluate(TokenList &rpnTokenList, float defaultValue, std::string &logMessage)
    {
		return evaluateReversePolishNotation(rpnTokenList, defaultValue, logMessage);
    }

    float ShuntingYard::evaluate(std::string const &expression, float defaultValue, std::string &logMessage)
    {
        TokenList rpnTokenList(getTokenList(expression, logMessage));
        return evaluateReversePolishNotation(rpnTokenList, defaultValue, logMessage);
    }

    bool ShuntingYard::isNumber(std::string const &token)
    {
        return std::regex_search(token, SearchNumber);
    }

    bool ShuntingYard::isOperation(std::string const &token)
    {
        return (operationsMap.count(token) > 0);
    }

    bool ShuntingYard::isFunction(std::string const &token)
    {
        return (functionsMap.count(token) > 0);
    }

    bool ShuntingYard::isLeftParenthesis(std::string const &token)
    {
        return (token.size() == 1 && token.at(0) == '(');
    }

    bool ShuntingYard::isRightParenthesis(std::string const &token)
    {
        return (token.size() == 1 && token.at(0) == ')');
    }

    bool ShuntingYard::isParenthesis(std::string const &token)
    {
        return (isLeftParenthesis(token) || isRightParenthesis(token));
    }

    bool ShuntingYard::isSeparator(std::string const &token)
    {
        return (token.size() == 1 && token.at(0) == ',');
    }

    bool ShuntingYard::isAssociative(std::string const &token, const Associations &type)
    {
        auto &p = operationsMap.find(token)->second;
        return p.association == type;
    }

    int ShuntingYard::comparePrecedence(std::string const &token1, std::string const &token2)
    {
        auto &p1 = operationsMap.find(token1)->second;
        auto &p2 = operationsMap.find(token2)->second;
        return p1.precedence - p2.precedence;
    }

    ShuntingYard::TokenType ShuntingYard::getTokenType(std::string const &token)
    {
        if (isSeparator(token))
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
        else if (isOperation(token))
        {
            return TokenType::BinaryOperation;
        }
        else if (isFunction(token))
        {
            return TokenType::Function;
        }
        else if (isNumber(token))
        {
            return TokenType::Number;
        }

        return TokenType::Unknown;
    }

    bool ShuntingYard::isValidReturnType(const Token &token)
    {
        switch (token.type)
        {
        case TokenType::Number:
        case TokenType::Function:
        case TokenType::UnaryOperation:
            return true;

        default:
            return false;
        };
    }

	bool ShuntingYard::insertToken(TokenList &infixTokenList, Token &token, std::string &logMessage)
    {
        if (!infixTokenList.empty())
        {
            const Token &previous = infixTokenList.back();

            // ) 2 or 2 2
            if (token.type == TokenType::Number && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(token.position, TokenType::BinaryOperation, "*"));
            }
            // 2 ( or ) (
            else if (token.type == TokenType::LeftParenthesis && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(token.position, TokenType::BinaryOperation, "*"));
            }
            // ) sin or 2 sin
            else if (token.type == TokenType::Function && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(token.position, TokenType::BinaryOperation, "*"));
            }
        }

        if (token.type == TokenType::BinaryOperation)
        {
            if (token.string.size() == 1 && (token.string.at(0) == '-' || token.string.at(0) == '+'))
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
		return true;
    }

    bool ShuntingYard::parseSubTokens(TokenList &infixTokenList, std::string const &token, size_t position, std::string &logMessage)
    {
        for (std::sregex_iterator tokenSearch(std::begin(token), std::end(token), SearchWord), end; tokenSearch != end; ++tokenSearch)
        {
            auto match = *tokenSearch;
            if (match[1].matched) // variable
            {
                std::string value(match.str(1));
                auto variableSearch = variableMap.find(value);
                if (variableSearch != std::end(variableMap))
                {
                    insertToken(infixTokenList, Token(position, variableSearch->second), logMessage);
                    continue;
                }

                auto functionSearch = functionsMap.find(value);
                if (functionSearch != std::end(functionsMap))
                {
                    insertToken(infixTokenList, Token(position, TokenType::Function, functionSearch->first), logMessage);
                    continue;
                }

				logMessage += String::Format("Unlisted variable/function name encountered: %v, at %v", token, position);
				return false;
            }
            else if(match[2].matched) // number
            {
                float value(String::Convert(match.str(2), 0.0f));
                insertToken(infixTokenList, Token(position, value), logMessage);
            }
        }

		return true;
    }

    ShuntingYard::TokenList ShuntingYard::convertExpressionToInfix(std::string const &expression, std::string &logMessage)
    {
        std::string runningToken;
        TokenList infixTokenList;
        for (size_t position = 0; position < expression.size(); ++position)
        {
            std::string nextToken(1U, expression.at(position));
            if (isOperation(nextToken) || isParenthesis(nextToken) || isSeparator(nextToken))
            {
                if (!runningToken.empty())
                {
                    parseSubTokens(infixTokenList, runningToken, position, logMessage);
                    runningToken.clear();
                }

                insertToken(infixTokenList, Token(position, getTokenType(nextToken), nextToken), logMessage);
            }
            else
            {
                if (nextToken.size() == 1 && nextToken.at(0) == ' ')
                {
                    if (!runningToken.empty())
                    {
                        parseSubTokens(infixTokenList, runningToken, position, logMessage);
                        runningToken.clear();
                    }
                }
                else
                {
                    runningToken.append(nextToken);
                }
            }
        }

        if (!runningToken.empty())
        {
            parseSubTokens(infixTokenList, runningToken, (expression.size() - runningToken.size()), logMessage);
        }

        return infixTokenList;
    }

    ShuntingYard::TokenList ShuntingYard::convertInfixToReversePolishNotation(const TokenList &infixTokenList, std::string &logMessage)
    {
        TokenList rpnTokenList;

        bool hasVector = false;

		std::stack<Token> tokenStack;
        std::stack<bool> parameterExistsStack;
		std::stack<uint32_t> parameterCountStack;
        for (const auto &token : infixTokenList)
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
				tokenStack.push(token);
                break;

            case TokenType::BinaryOperation:
                while (!tokenStack.empty() && (tokenStack.top().type == TokenType::BinaryOperation &&
                    (isAssociative(token.string, Associations::Left) && comparePrecedence(token.string, tokenStack.top().string) == 0) ||
                    (isAssociative(token.string, Associations::Right) && comparePrecedence(token.string, tokenStack.top().string) < 0)))
                {
                    rpnTokenList.push_back(PopTop(tokenStack));
                };

                tokenStack.push(token);
                break;

            case TokenType::LeftParenthesis:
                // only return vector as a final value
                if (tokenStack.empty() || (tokenStack.top().type == TokenType::Function))
                {
                    parameterCountStack.push(0);
                    if (!parameterExistsStack.empty())
                    {
                        parameterExistsStack.pop();
                        parameterExistsStack.push(true);
                    }

                    parameterExistsStack.push(false);
                }

                tokenStack.push(token);
                break;

            case TokenType::RightParenthesis:
                if (tokenStack.empty())
                {
                    logMessage += String::Format("Unmatched right parenthesis found at %v", token.position);
					return EmptyTokenList;
                }

                while (tokenStack.top().type != TokenType::LeftParenthesis)
                {
                    rpnTokenList.push_back(PopTop(tokenStack));
                    if (tokenStack.empty())
                    {
                        logMessage += String::Format("Unmatched right parenthesis found at %v", token.position);
						return EmptyTokenList;
					}
                };

                tokenStack.pop();
                if (!tokenStack.empty() && tokenStack.top().type == TokenType::Function)
                {
                    Token function = PopTop(tokenStack);
					function.parameterCount = PopTop(parameterCountStack);
                    if (PopTop(parameterExistsStack))
                    {
                        function.parameterCount++;
                    }

                    rpnTokenList.push_back(function);
                }

                break;

            case TokenType::Separator:
                if (tokenStack.empty())
                {
                    logMessage += String::Format("Separator encountered outside of parenthesis block at %v", token.position);
					return EmptyTokenList;
				}

                if (parameterExistsStack.empty())
                {
                    logMessage += String::Format("Separator encountered at start of parenthesis block at %v", token.position);
					return EmptyTokenList;
				}

                while (tokenStack.top().type != TokenType::LeftParenthesis)
                {
                    rpnTokenList.push_back(PopTop(tokenStack));
                    if (tokenStack.empty())
                    {
                        logMessage += String::Format("Separator encountered without leading left parenthesis at %v", token.position);
						return EmptyTokenList;
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
                tokenStack.push(token);
                break;

            default:
                logMessage += String::Format("Unknown token type encountered at %v", token.position);
				return EmptyTokenList;
			};
        }

        while (!tokenStack.empty())
        {
			auto &top = tokenStack.top();
            if (top.type == TokenType::LeftParenthesis || top.type == TokenType::RightParenthesis)
            {
                logMessage += String::Format("Invalid surrounding parenthesis encountered at %v", top.position);
				return EmptyTokenList;
			}

            rpnTokenList.push_back(PopTop(tokenStack));
        };

        if (rpnTokenList.empty())
        {
			logMessage += String::Format("Empty equation encountered");
			return EmptyTokenList;
		}

        return rpnTokenList;
    }

	float ShuntingYard::evaluateReversePolishNotation(const TokenList &rpnTokenList, float defaultValue, std::string &logMessage)
    {
		std::stack<float> stack;
        for (const auto &token : rpnTokenList)
        {
            switch (token.type)
            {
            case TokenType::Number:
                stack.push(token.value);
                break;

            case TokenType::UnaryOperation:
                if (true)
                {
                    auto &operationSearch = operationsMap.find(token.string);
                    if (operationSearch == std::end(operationsMap))
                    {
                        logMessage += String::Format("Unlisted unary operation encountered at %v", token.position);
						return defaultValue;
                    }

                    auto &operation = operationSearch->second;
                    if (!operation.unaryFunction)
                    {
                        logMessage += String::Format("Missing unary function encountered at %v", token.position);
						return defaultValue;
					}

                    if (stack.empty())
                    {
                        logMessage += String::Format("Unary function encountered without parameter at %v", token.position);
						return defaultValue;
					}

                    float functionValue = PopTop(stack);
                    stack.push(operation.unaryFunction(functionValue));
                    break;
                }

            case TokenType::BinaryOperation:
                if (true)
                {
                    auto &operationSearch = operationsMap.find(token.string);
                    if (operationSearch == std::end(operationsMap))
                    {
                        logMessage += String::Format("Unlisted binary operation encounterd at %v", token.position);
						return defaultValue;
					}

                    auto &operation = operationSearch->second;
                    if (!operation.binaryFunction)
                    {
                        logMessage += String::Format("Missing binary function encountered at %v", token.position);
						return defaultValue;
					}

                    if (stack.empty())
                    {
                        logMessage += String::Format("Binary function used without first parameter at %v", token.position);
						return defaultValue;
					}

                    float functionValueRight = PopTop(stack);
                    if (stack.empty())
                    {
                        logMessage += String::Format("Binary function used without second parameter at %v", token.position);
						return defaultValue;
					}

                    float functionValueLeft = PopTop(stack);
                    stack.push(operation.binaryFunction(functionValueLeft, functionValueRight));
                    break;
                }

            case TokenType::Function:
                if (true)
                {
                    auto &functionSearch = functionsMap.find(token.string);
                    if (functionSearch == std::end(functionsMap))
                    {
                        logMessage += String::Format("Unlisted function encountered at %v", token.position);
						return defaultValue;
					}

                    auto &function = functionSearch->second;
                    if (function.parameterCount != token.parameterCount)
                    {
                        logMessage += String::Format("Expected different number of parameters for function at %v", token.position);
						return defaultValue;
					}

                    if (stack.size() < function.parameterCount)
                    {
                        logMessage += String::Format("Not enough parameters passed to function at %v", token.position);
						return defaultValue;
					}

                    stack.push(function.function(stack));
                    break;
                }

            default:
                logMessage += String::Format("Unknown token type encountered at %v", token.position);
				return defaultValue;
			};
        }

        if (rpnTokenList.empty())
        {
			logMessage += String::Format("Empty equation encountered");
			return defaultValue;
		}

        if (stack.size() != 1)
        {
			logMessage += String::Format("Too many values left in stack: %v", stack.size());
			return defaultValue;
		}

		return stack.top();
    }
}; // namespace Gek
