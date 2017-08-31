#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility//Hash.hpp"
#include "GEK/Math/Common.hpp"

// https://blog.kallisti.net.rz.xyz/2008/02/extension-to-the-shunting-yard-algorithm-to-allow-variable-numbers-of-arguments-to-functions/
namespace Gek
{
	template <typename TYPE>
	TYPE PopTop(std::stack<TYPE> &stack)
	{
		auto top = stack.top();
		stack.pop();
		return top;
	}

	ShuntingYard::Token::Token(ShuntingYard::TokenType type)
        : type(type)
    {
    }

    ShuntingYard::Token::Token(ShuntingYard::TokenType type, std::string const &string, uint32_t parameterCount)
        : type(type)
        , string(string)
        , parameterCount(parameterCount)
    {
    }

    ShuntingYard::Token::Token(float value)
        : type(TokenType::Number)
        , value(value)
    {
    }

    ShuntingYard::Token::Token(float *variable)
        : type(TokenType::Number)
        , variable(variable)
    {
    }

    ShuntingYard::Operand::Operand(void)
        : type(OperandType::Unknown)
    {
    }

    ShuntingYard::Operand::Operand(Token const &token)
    {
        if (token.variable)
        {
            type = OperandType::Variable;
            variable = token.variable;
        }
        else
        {
            type = OperandType::Number;
            value = token.value;
        }
    }

    ShuntingYard::Operand::Operand(Token const &token, Operation *operation)
    {
        type = (token.type == TokenType::UnaryOperation ? OperandType::UnaryOperation : OperandType::BinaryOperation);
        this->operation = operation;
    }

    ShuntingYard::Operand::Operand(Token const &token, Function *function)
    {
        type = OperandType::Function;
        this->function = function;
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

    ShuntingYard::ShuntingYard(ShuntingYard const &shuntingYard)
        : seed(shuntingYard.seed)
        , variableMap(shuntingYard.variableMap)
        , operationsMap(shuntingYard.operationsMap)
        , functionsMap(shuntingYard.functionsMap)
        , mersineTwister(shuntingYard.mersineTwister)
        , cache(shuntingYard.cache)
    {
    }

    void ShuntingYard::setVariable(std::string const &name, float value)
    {
        variableMap[name] = value;
    }

    void ShuntingYard::setOperation(std::string const &name, int precedence, Associations association, std::function<float(float value)> &unaryFunction, std::function<float(float valueLeft, float valueRight)> &binaryFunction)
    {
        operationsMap[name] = { precedence, association, unaryFunction, binaryFunction };
    }

    void ShuntingYard::setFunction(std::string const &name, uint32_t parameterCount, std::function<float(std::stack<float> &)> &function)
    {
        functionsMap[name] = { parameterCount, function };
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

    std::optional<ShuntingYard::OperandList> ShuntingYard::getTokenList(std::string const &expression)
    {
        const auto hash = GetHash(expression);
        auto cacheSearch = cache.find(hash);
        if (cacheSearch != std::end(cache))
        {
            return cacheSearch->second;
        }

		auto infixTokenList(convertExpressionToInfix(expression));
        if (infixTokenList)
        {
            auto tokenList(convertInfixToReversePolishNotation(infixTokenList.value()));
            if (tokenList)
            {
                return cache.insert(std::make_pair(hash, tokenList.value())).first->second;
            }
        }

        return std::nullopt;
    }

    std::optional<float> ShuntingYard::evaluate(OperandList &rpOperandList)
    {
		return evaluateReversePolishNotation(rpOperandList);
    }

    std::optional<float> ShuntingYard::evaluate(std::string const &expression)
    {
        auto rpnOperandList = getTokenList(expression);
        if (rpnOperandList)
        {
            return evaluateReversePolishNotation(rpnOperandList.value());
        }

        return std::nullopt;
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

    ShuntingYard::Operand ShuntingYard::getOperand(Token const &token)
    {
        switch (token.type)
        {
        case TokenType::Number:
            return Operand(token);

        case TokenType::UnaryOperation:
        case TokenType::BinaryOperation:
            if (true)
            {
                auto &operationSearch = operationsMap.find(token.string);
                if (operationSearch == std::end(operationsMap))
                {
                    return Operand();
                }

                auto &operation = operationSearch->second;
                return Operand(token, &operation);
            }

        case TokenType::Function:
            if (true)
            {
                auto &functionSearch = functionsMap.find(token.string);
                if (functionSearch == std::end(functionsMap))
                {
                    return Operand();
                }

                auto &function = functionSearch->second;
                if (function.parameterCount != token.parameterCount)
                {
                    return Operand();
                }

                return Operand(token, &function);
            }
        };

        return Operand();
    }

	bool ShuntingYard::insertToken(TokenList &infixTokenList, Token &token)
    {
        if (!infixTokenList.empty())
        {
            const Token &previous = infixTokenList.back();

            // ) 2 or 2 2
            if (token.type == TokenType::Number && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::BinaryOperation, "*"));
            }
            // 2 ( or ) (
            else if (token.type == TokenType::LeftParenthesis && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::BinaryOperation, "*"));
            }
            // ) sin or 2 sin
            else if (token.type == TokenType::Function && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::BinaryOperation, "*"));
            }
        }

        if (token.type == TokenType::BinaryOperation)
        {
            if (token.string.size() == 1)
            {
                if (token.string.at(0) == '-' || token.string.at(0) == '+')
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
        }

        infixTokenList.push_back(token);
		return true;
    }

    static const auto locale = std::locale::classic();
    std::optional<ShuntingYard::TokenList> ShuntingYard::convertExpressionToInfix(std::string const &expression)
    {
        std::string runningToken;
        TokenList infixTokenList;
        auto insertWord = [&](void)
        {
            auto variableSearch = variableMap.find(runningToken);
            if (variableSearch != std::end(variableMap))
            {
                insertToken(infixTokenList, Token(&variableSearch->second));
            }

            auto functionSearch = functionsMap.find(runningToken);
            if (functionSearch != std::end(functionsMap))
            {
                insertToken(infixTokenList, Token(TokenType::Function, functionSearch->first));
            }
        };

        auto insertNumber = [&](void)
        {
            insertToken(infixTokenList, Token(String::Convert(runningToken, 0.0f)));
        };

        auto insertOperation = [&](void)
        {
            auto operationSearch = operationsMap.find(runningToken);
            if (operationSearch != std::end(operationsMap))
            {
                insertToken(infixTokenList, Token(TokenType::BinaryOperation, operationSearch->first));
            }
        };

        enum class RunningType : uint8_t
        {
            None = 0,
            Word,
            Number,
            Operation,
        };

        auto getStartingType = [&](char nextCharacter) -> RunningType
        {
            if (std::isalpha(nextCharacter, locale))
            {
                return RunningType::Word;
            }
            else if (nextCharacter == '-' ||
                nextCharacter == '+' ||
                nextCharacter == '.' ||
                std::isdigit(nextCharacter, locale))
            {
                return RunningType::Number;
            }

            return RunningType::Operation;
        };

        auto getNextType = [&](char nextCharacter) -> RunningType
        {
            if (std::isalpha(nextCharacter, locale))
            {
                return RunningType::Word;
            }
            else if (nextCharacter == '-' ||
                nextCharacter == '+' ||
                nextCharacter == '.' ||
                nextCharacter == 'e' ||
                nextCharacter == 'E' ||
                std::isdigit(nextCharacter, locale))
            {
                return RunningType::Number;
            }

            return RunningType::Operation;
        };

        RunningType runningType = RunningType::None;
        auto insertRunningType = [&](void)
        {
            switch (runningType)
            {
            case RunningType::Word:
                insertWord();
                break;

            case RunningType::Number:
                insertNumber();
                break;

            case RunningType::Operation:
                insertOperation();
                break;
            };

            runningToken.clear();
        };

        for (auto nextCharacter : expression)
        {
            if (nextCharacter == '(' ||
                nextCharacter == ')' ||
                nextCharacter == ',' ||
                nextCharacter == ' ')
            {
                if (!runningToken.empty())
                {
                    insertRunningType();
                }

                switch (nextCharacter)
                {
                case '(':
                    insertToken(infixTokenList, Token(TokenType::LeftParenthesis));
                    break;

                case ')':
                    insertToken(infixTokenList, Token(TokenType::RightParenthesis));
                    break;

                case ',':
                    insertToken(infixTokenList, Token(TokenType::Separator));
                    break;
                };
            }
            else if (runningToken.empty())
            {
                runningType = getStartingType(nextCharacter);
                runningToken += nextCharacter;
            }
            else
            {
                auto nextRunningType = getNextType(nextCharacter);
                if (nextRunningType != runningType)
                {
                    insertRunningType();
                    runningType = nextRunningType;
                }

                runningToken += nextCharacter;
            }
        }

        if (!runningToken.empty())
        {
            insertRunningType();
        }

        return infixTokenList;
    }

    std::optional<ShuntingYard::OperandList> ShuntingYard::convertInfixToReversePolishNotation(const TokenList &infixTokenList)
    {
        OperandList rpnOperandList;
		std::stack<Token> tokenStack;
        std::stack<bool> parameterExistsStack;
		std::stack<uint32_t> parameterCountStack;
        for (auto const &token : infixTokenList)
        {
            switch (token.type)
            {
            case TokenType::Number:
                if (true)
                {
                    rpnOperandList.push_back(getOperand(token));
                    if (!parameterExistsStack.empty())
                    {
                        parameterExistsStack.pop();
                        parameterExistsStack.push(true);
                    }

                    break;
                }

            case TokenType::UnaryOperation:
				tokenStack.push(token);
                break;

            case TokenType::BinaryOperation:
                while (!tokenStack.empty() && (tokenStack.top().type == TokenType::BinaryOperation &&
                    (isAssociative(token.string, Associations::Left) && comparePrecedence(token.string, tokenStack.top().string) == 0) ||
                    (isAssociative(token.string, Associations::Right) && comparePrecedence(token.string, tokenStack.top().string) < 0)))
                {
                    rpnOperandList.push_back(getOperand(PopTop(tokenStack)));
                };

                tokenStack.push(token);
                break;

            case TokenType::LeftParenthesis:
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
                    return std::nullopt;
                }

                while (tokenStack.top().type != TokenType::LeftParenthesis)
                {
                    rpnOperandList.push_back(getOperand(PopTop(tokenStack)));
                    if (tokenStack.empty())
                    {
                        return std::nullopt;
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

                    rpnOperandList.push_back(getOperand(function));
                }

                break;

            case TokenType::Separator:
                if (tokenStack.empty())
                {
                    return std::nullopt;
                }

                if (parameterExistsStack.empty())
                {
					return std::nullopt;
				}

                while (tokenStack.top().type != TokenType::LeftParenthesis)
                {
                    rpnOperandList.push_back(getOperand(PopTop(tokenStack)));
                    if (tokenStack.empty())
                    {
						return std::nullopt;
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
				return std::nullopt;
			};
        }

        while (!tokenStack.empty())
        {
			auto &top = tokenStack.top();
            if (top.type == TokenType::LeftParenthesis || top.type == TokenType::RightParenthesis)
            {
				return std::nullopt;
			}

            rpnOperandList.push_back(getOperand(PopTop(tokenStack)));
        };

        if (rpnOperandList.empty())
        {
			return std::nullopt;
		}

        return rpnOperandList;
    }

    std::optional<float> ShuntingYard::evaluateReversePolishNotation(const OperandList &rpnOperandList)
    {
        if (rpnOperandList.empty())
        {
            return std::nullopt;
        }

        std::stack<float> stack;
        for (auto const &operand : rpnOperandList)
        {
            switch (operand.type)
            {
            case OperandType::Number:
                stack.push(operand.value);
                break;

            case OperandType::Variable:
                stack.push(*operand.variable);
                break;

            case OperandType::UnaryOperation:
                if (true)
                {
                    if (stack.empty())
                    {
						return std::nullopt;
					}

                    float functionValue = PopTop(stack);
                    stack.push(operand.operation->unaryFunction(functionValue));
                    break;
                }

            case OperandType::BinaryOperation:
                if (true)
                {
                    if (stack.empty())
                    {
						return std::nullopt;
					}

                    float functionValueRight = PopTop(stack);
                    if (stack.empty())
                    {
						return std::nullopt;
					}

                    float functionValueLeft = PopTop(stack);
                    stack.push(operand.operation->binaryFunction(functionValueLeft, functionValueRight));
                    break;
                }

            case OperandType::Function:
                if (true)
                {
                    if (stack.size() < operand.function->parameterCount)
                    {
						return std::nullopt;
					}

                    stack.push(operand.function->function(stack));
                    break;
                }

            default:
				return std::nullopt;
			};
        }

        if (stack.empty())
        {
            return std::nullopt;
        }
        else if (stack.size() != 1)
        {
			return std::nullopt;
		}

		return stack.top();
    }
}; // namespace Gek
