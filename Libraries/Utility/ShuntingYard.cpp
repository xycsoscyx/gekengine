#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Utility/String.hpp"
#include <regex>

// https://blog.kallisti.net.rz.xyz/2008/02/extension-to-the-shunting-yard-algorithm-to-allow-variable-numbers-of-arguments-to-functions/

namespace Gek
{
    #define SEARCH_NUMBER L"([+-]?(?:(?:\\d+(?:\\.\\d*)?)|(?:\\.\\d+))(?:e\\d+)?)"

    static const std::wregex SearchNumber(SEARCH_NUMBER, std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
    static const std::wregex SearchWord(L"([a-z]+)|" SEARCH_NUMBER, std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
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

    ShuntingYard::Token::Token(size_t position, ShuntingYard::TokenType type, WString const &string, uint32_t parameterCount)
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
        variableMap[L"pi"] = Math::Pi;
        variableMap[L"tau"] = Math::Tau;
        variableMap[L"e"] = Math::E;
        variableMap[L"true"] = 1.0f;
        variableMap[L"false"] = 0.0f;

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

        functionsMap.insert({ L"sin",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = stack.top();
            return std::sin(value);
        } } });

        functionsMap.insert({ L"cos",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::cos(value);
        } } });

        functionsMap.insert({ L"tan",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::tan(value);
        } } });

        functionsMap.insert({ L"asin",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::asin(value);
        } } });

        functionsMap.insert({ L"acos",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::acos(value);
        } } });

        functionsMap.insert({ L"atan",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::atan(value);
        } } });

        functionsMap.insert({ L"min",{ 2, [](std::stack<float> &stack) -> float
        {
            float value2 = PopTop(stack);
            float value1 = PopTop(stack);
            return std::min(value1, value2);
        } } });

        functionsMap.insert({ L"max",{ 2, [](std::stack<float> &stack) -> float
        {
            float value2 = PopTop(stack);
            float value1 = PopTop(stack);
            return std::max(value1, value2);
        } } });

        functionsMap.insert({ L"abs",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::abs(value);
        } } });

        functionsMap.insert({ L"ceil",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::ceil(value);
        } } });

        functionsMap.insert({ L"floor",{ 1, [](std::stack<float> &stack) -> float
        {
            float value = PopTop(stack);
            return std::floor(value);
        } } });

        functionsMap.insert({ L"lerp",{ 3, [](std::stack<float> &stack) -> float
        {
            float value3 = PopTop(stack);
            float value2 = PopTop(stack);
            float value1 = PopTop(stack);
            return Math::Interpolate(value1, value2, value3);
        } } });

        functionsMap.insert({ L"random",{ 2, [&](std::stack<float> &stack) -> float
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

    ShuntingYard::TokenList ShuntingYard::getTokenList(WString const &expression, WString &logMessage)
    {
		TokenList infixTokenList(convertExpressionToInfix(expression, logMessage));
        return convertInfixToReversePolishNotation(infixTokenList, logMessage);
    }

    float ShuntingYard::evaluate(TokenList &rpnTokenList, float defaultValue, WString &logMessage)
    {
		return evaluateReversePolishNotation(rpnTokenList, defaultValue, logMessage);
    }

    float ShuntingYard::evaluate(WString const &expression, float defaultValue, WString &logMessage)
    {
        TokenList rpnTokenList(getTokenList(expression, logMessage));
        return evaluateReversePolishNotation(rpnTokenList, defaultValue, logMessage);
    }

    bool ShuntingYard::isNumber(WString const &token)
    {
        return std::regex_search(token, SearchNumber);
    }

    bool ShuntingYard::isOperation(WString const &token)
    {
        return (operationsMap.count(token) > 0);
    }

    bool ShuntingYard::isFunction(WString const &token)
    {
        return (functionsMap.count(token) > 0);
    }

    bool ShuntingYard::isLeftParenthesis(WString const &token)
    {
        return (token == L"(");
    }

    bool ShuntingYard::isRightParenthesis(WString const &token)
    {
        return (token == L")");
    }

    bool ShuntingYard::isParenthesis(WString const &token)
    {
        return (isLeftParenthesis(token) || isRightParenthesis(token));
    }

    bool ShuntingYard::isSeparator(WString const &token)
    {
        return (token == L",");
    }

    bool ShuntingYard::isAssociative(WString const &token, const Associations &type)
    {
        auto &p = operationsMap.find(token)->second;
        return p.association == type;
    }

    int ShuntingYard::comparePrecedence(WString const &token1, WString const &token2)
    {
        auto &p1 = operationsMap.find(token1)->second;
        auto &p2 = operationsMap.find(token2)->second;
        return p1.precedence - p2.precedence;
    }

    ShuntingYard::TokenType ShuntingYard::getTokenType(WString const &token)
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

	bool ShuntingYard::insertToken(TokenList &infixTokenList, Token &token, WString &logMessage)
    {
        if (!infixTokenList.empty())
        {
            const Token &previous = infixTokenList.back();

            // ) 2 or 2 2
            if (token.type == TokenType::Number && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(token.position, TokenType::BinaryOperation, L"*"));
            }
            // 2 ( or ) (
            else if (token.type == TokenType::LeftParenthesis && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(token.position, TokenType::BinaryOperation, L"*"));
            }
            // ) sin or 2 sin
            else if (token.type == TokenType::Function && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(token.position, TokenType::BinaryOperation, L"*"));
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
		return true;
    }

    bool ShuntingYard::parseSubTokens(TokenList &infixTokenList, WString const &token, size_t position, WString &logMessage)
    {
        for (std::wsregex_iterator tokenSearch(std::begin(token), std::end(token), SearchWord), end; tokenSearch != end; ++tokenSearch)
        {
            auto match = *tokenSearch;
            if (match[1].matched) // variable
            {
                WString value(match.str(1));
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

				logMessage.appendFormat(L"Unlisted variable/function name encountered: %v, at %v", token, position);
				return false;
            }
            else if(match[2].matched) // number
            {
                float value(WString(match.str(2)));
                insertToken(infixTokenList, Token(position, value), logMessage);
            }
        }

		return true;
    }

    ShuntingYard::TokenList ShuntingYard::convertExpressionToInfix(WString const &expression, WString &logMessage)
    {
        WString runningToken;
        TokenList infixTokenList;
        for (size_t position = 0; position < expression.size(); ++position)
        {
            WString nextToken(expression.subString(position, 1));
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
                if (nextToken == L" ")
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

    ShuntingYard::TokenList ShuntingYard::convertInfixToReversePolishNotation(const TokenList &infixTokenList, WString &logMessage)
    {
        TokenList rpnTokenList;

        bool hasVector = false;

		std::stack<Token> tokenStack;
        std::stack<bool> parameterExistsStack;
		std::stack<uint32_t> parameterCountStack;
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
                    logMessage.appendFormat(L"Unmatched right parenthesis found at %v", token.position);
					return EmptyTokenList;
                }

                while (tokenStack.top().type != TokenType::LeftParenthesis)
                {
                    rpnTokenList.push_back(PopTop(tokenStack));
                    if (tokenStack.empty())
                    {
                        logMessage.appendFormat(L"Unmatched right parenthesis found at %v", token.position);
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
                    logMessage.appendFormat(L"Separator encountered outside of parenthesis block at %v", token.position);
					return EmptyTokenList;
				}

                if (parameterExistsStack.empty())
                {
                    logMessage.appendFormat(L"Separator encountered at start of parenthesis block at %v", token.position);
					return EmptyTokenList;
				}

                while (tokenStack.top().type != TokenType::LeftParenthesis)
                {
                    rpnTokenList.push_back(PopTop(tokenStack));
                    if (tokenStack.empty())
                    {
                        logMessage.appendFormat(L"Separator encountered without leading left parenthesis at %v", token.position);
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
                logMessage.appendFormat(L"Unknown token type encountered at %v", token.position);
				return EmptyTokenList;
			};
        }

        while (!tokenStack.empty())
        {
			auto &top = tokenStack.top();
            if (top.type == TokenType::LeftParenthesis || top.type == TokenType::RightParenthesis)
            {
                logMessage.appendFormat(L"Invalid surrounding parenthesis encountered at %v", top.position);
				return EmptyTokenList;
			}

            rpnTokenList.push_back(PopTop(tokenStack));
        };

        if (rpnTokenList.empty())
        {
			logMessage.appendFormat(L"Empty equation encountered");
			return EmptyTokenList;
		}

        return rpnTokenList;
    }

	float ShuntingYard::evaluateReversePolishNotation(const TokenList &rpnTokenList, float defaultValue, WString &logMessage)
    {
		std::stack<float> stack;
        for (auto &token : rpnTokenList)
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
                        logMessage.appendFormat(L"Unlisted unary operation encountered at %v", token.position);
						return defaultValue;
                    }

                    auto &operation = operationSearch->second;
                    if (!operation.unaryFunction)
                    {
                        logMessage.appendFormat(L"Missing unary function encountered at %v", token.position);
						return defaultValue;
					}

                    if (stack.empty())
                    {
                        logMessage.appendFormat(L"Unary function encountered without parameter at %v", token.position);
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
                        logMessage.appendFormat(L"Unlisted binary operation encounterd at %v", token.position);
						return defaultValue;
					}

                    auto &operation = operationSearch->second;
                    if (!operation.binaryFunction)
                    {
                        logMessage.appendFormat(L"Missing binary function encountered at %v", token.position);
						return defaultValue;
					}

                    if (stack.empty())
                    {
                        logMessage.appendFormat(L"Binary function used without first parameter at %v", token.position);
						return defaultValue;
					}

                    float functionValueRight = PopTop(stack);
                    if (stack.empty())
                    {
                        logMessage.appendFormat(L"Binary function used without second parameter at %v", token.position);
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
                        logMessage.appendFormat(L"Unlisted function encountered at %v", token.position);
						return defaultValue;
					}

                    auto &function = functionSearch->second;
                    if (function.parameterCount != token.parameterCount)
                    {
                        logMessage.appendFormat(L"Expected different number of parameters for function at %v", token.position);
						return defaultValue;
					}

                    if (stack.size() < function.parameterCount)
                    {
                        logMessage.appendFormat(L"Not enough parameters passed to function at %v", token.position);
						return defaultValue;
					}

                    stack.push(function.function(stack));
                    break;
                }

            default:
                logMessage.appendFormat(L"Unknown token type encountered at %v", token.position);
				return defaultValue;
			};
        }

        if (rpnTokenList.empty())
        {
			logMessage.appendFormat(L"Empty equation encountered");
			return defaultValue;
		}

        if (stack.size() != 1)
        {
			logMessage.appendFormat(L"Too many values left in stack: %v", stack.size());
			return defaultValue;
		}

		return stack.top();
    }
}; // namespace Gek
