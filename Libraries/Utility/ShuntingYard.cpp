#include "GEK/Utility/ShuntingYard.hpp"
#include "GEK/Utility/String.hpp"
#include <unordered_map>
#include <functional>
#include <random>
#include <stack>
#include <regex>

// https://blog.kallisti.net.rz.xyz/2008/02/extension-to-the-shunting-yard-algorithm-to-allow-variable-numbers-of-arguments-to-functions/

namespace Gek
{
    #define SEARCH_NUMBER L"([+-]?(?:(?:\\d+(?:\\.\\d*)?)|(?:\\.\\d+))(?:e\\d+)?)"

    static const std::wregex SearchNumber(SEARCH_NUMBER, std::regex::ECMAScript | std::regex::icase | std::regex::optimize);
    static const std::wregex SearchWord(L"([a-z]+)|" SEARCH_NUMBER, std::regex::ECMAScript | std::regex::icase | std::regex::optimize);

    ShuntingYard::Token::Token(ShuntingYard::TokenType type)
        : type(type)
        , parameterCount(0)
    {
    }

    ShuntingYard::Token::Token(ShuntingYard::TokenType type, const wchar_t *string, uint32_t parameterCount)
        : type(type)
        , string(string)
        , parameterCount(parameterCount)
    {
    }

    ShuntingYard::Token::Token(float value)
        : type(TokenType::Number)
        , value(value)
        , parameterCount(1)
    {
    }
    
    ShuntingYard::ShuntingYard(void)
        : mersineTwister(std::random_device()())
    {
        variableMap[L"pi"] = Math::Pi;
        variableMap[L"tau"] = Math::Tau;
        variableMap[L"e"] = Math::E;

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

        functionsMap.insert({ L"sin",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::sin(value);
        } } });

        functionsMap.insert({ L"cos",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::cos(value);
        } } });

        functionsMap.insert({ L"tan",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::tan(value);
        } } });

        functionsMap.insert({ L"asin",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::asin(value);
        } } });

        functionsMap.insert({ L"acos",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::acos(value);
        } } });

        functionsMap.insert({ L"atan",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::atan(value);
        } } });

        functionsMap.insert({ L"min",{ 2, [](Stack<Token> &stack) -> float
        {
            float value2 = stack.popTop().value;
            float value1 = stack.popTop().value;
            return std::min(value1, value2);
        } } });

        functionsMap.insert({ L"max",{ 2, [](Stack<Token> &stack) -> float
        {
            float value2 = stack.popTop().value;
            float value1 = stack.popTop().value;
            return std::max(value1, value2);
        } } });

        functionsMap.insert({ L"abs",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::abs(value);
        } } });

        functionsMap.insert({ L"ceil",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::ceil(value);
        } } });

        functionsMap.insert({ L"floor",{ 1, [](Stack<Token> &stack) -> float
        {
            float value = stack.popTop().value;
            return std::floor(value);
        } } });

        functionsMap.insert({ L"lerp",{ 3, [](Stack<Token> &stack) -> float
        {
            float value3 = stack.popTop().value;
            float value2 = stack.popTop().value;
            float value1 = stack.popTop().value;
            return Math::Interpolate(value1, value2, value3);
        } } });

        functionsMap.insert({ L"random",{ 2, [&](Stack<Token> &stack) -> float
        {
            float value2 = stack.popTop().value;
            float value1 = stack.popTop().value;
            std::uniform_real_distribution<float> uniformRealDistribution(value1, value2);
            return uniformRealDistribution(mersineTwister);
        } } });
    }

    void ShuntingYard::setRandomSeed(uint32_t seed)
    {
        mersineTwister.seed(seed);
    }

    ShuntingYard::TokenList ShuntingYard::getTokenList(const wchar_t *expression)
    {
        TokenList infixTokenList = convertExpressionToInfix(expression);
        return convertInfixToReversePolishNotation(infixTokenList);
    }

    float ShuntingYard::evaluate(TokenList &rpnTokenList)
    {
        return evaluateReversePolishNotation(rpnTokenList);
    }

    float ShuntingYard::evaluate(const wchar_t *expression)
    {
        TokenList rpnTokenList(getTokenList(expression));
        return evaluateReversePolishNotation(rpnTokenList);
    }

    bool ShuntingYard::isNumber(const wchar_t *token)
    {
        return std::regex_search(token, SearchNumber);
    }

    bool ShuntingYard::isOperation(const wchar_t *token)
    {
        return (operationsMap.count(token) > 0);
    }

    bool ShuntingYard::isFunction(const wchar_t *token)
    {
        return (functionsMap.count(token) > 0);
    }

    bool ShuntingYard::isLeftParenthesis(const wchar_t *token)
    {
        return (token && *token == L'(');
    }

    bool ShuntingYard::isRightParenthesis(const wchar_t *token)
    {
        return (token && *token == L')');
    }

    bool ShuntingYard::isParenthesis(const wchar_t *token)
    {
        return (isLeftParenthesis(token) || isRightParenthesis(token));
    }

    bool ShuntingYard::isSeparator(const wchar_t *token)
    {
        return (token && *token == L',');
    }

    bool ShuntingYard::isAssociative(const wchar_t *token, const Associations &type)
    {
        auto &p = operationsMap.find(token)->second;
        return p.association == type;
    }

    int ShuntingYard::comparePrecedence(const wchar_t *token1, const wchar_t *token2)
    {
        auto &p1 = operationsMap.find(token1)->second;
        auto &p2 = operationsMap.find(token2)->second;
        return p1.precedence - p2.precedence;
    }

    ShuntingYard::TokenType ShuntingYard::getTokenType(const wchar_t *token)
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

    void ShuntingYard::insertToken(TokenList &infixTokenList, Token &token)
    {
        if (!infixTokenList.empty())
        {
            const Token &previous = infixTokenList.back();

            // ) 2 or 2 2
            if (token.type == TokenType::Number && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::BinaryOperation, L"*"));
            }
            // 2 ( or ) (
            else if (token.type == TokenType::LeftParenthesis && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::BinaryOperation, L"*"));
            }
            // ) sin or 2 sin
            else if (token.type == TokenType::Function && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::BinaryOperation, L"*"));
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
    }

    void ShuntingYard::parseSubTokens(TokenList &infixTokenList, const String &token)
    {
        for (std::wsregex_iterator tokenSearch(std::begin(token), std::end(token), SearchWord), end; tokenSearch != end; ++tokenSearch)
        {
            auto match = *tokenSearch;
            if (match[1].matched) // variable
            {
                String value(match.str(1));
                auto variableSearch = variableMap.find(value);
                if (variableSearch != std::end(variableMap))
                {
                    insertToken(infixTokenList, Token(variableSearch->second));
                    continue;
                }

                auto functionSearch = functionsMap.find(value);
                if (functionSearch != std::end(functionsMap))
                {
                    insertToken(infixTokenList, Token(TokenType::Function, functionSearch->first));
                    continue;
                }

                throw UnknownTokenType("Unlisted variable/function name encountered");
            }
            else if(match[2].matched) // number
            {
                float value(String(match.str(2)));
                insertToken(infixTokenList, Token(value));
            }
        }
    }

    ShuntingYard::TokenList ShuntingYard::convertExpressionToInfix(const String &expression)
    {
        String runningToken;
        TokenList infixTokenList;
        for (size_t index = 0; index < expression.size(); ++index)
        {
            String nextToken(expression.subString(index, 1));
            if (isOperation(nextToken) || isParenthesis(nextToken) || isSeparator(nextToken))
            {
                if (!runningToken.empty())
                {
                    parseSubTokens(infixTokenList, runningToken);
                    runningToken.clear();
                }

                insertToken(infixTokenList, Token(getTokenType(nextToken), nextToken));
            }
            else
            {
                if (nextToken == L" ")
                {
                    if (!runningToken.empty())
                    {
                        parseSubTokens(infixTokenList, runningToken);
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
            parseSubTokens(infixTokenList, runningToken);
        }

        return infixTokenList;
    }

    ShuntingYard::TokenList ShuntingYard::convertInfixToReversePolishNotation(const TokenList &infixTokenList)
    {
        TokenList rpnTokenList;

        bool hasVector = false;

        Stack<Token> stack;
        Stack<bool> parameterExistsStack;
        Stack<uint32_t> parameterCountStack;
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
                stack.push(token);
                break;

            case TokenType::BinaryOperation:
                while (!stack.empty() && (stack.top().type == TokenType::BinaryOperation &&
                    (isAssociative(token.string, Associations::Left) && comparePrecedence(token.string, stack.top().string) == 0) ||
                    (isAssociative(token.string, Associations::Right) && comparePrecedence(token.string, stack.top().string) < 0)))
                {
                    rpnTokenList.push_back(stack.popTop());
                };

                stack.push(token);
                break;

            case TokenType::LeftParenthesis:
                // only return vector as a final value
                if (stack.empty() || (stack.top().type == TokenType::Function))
                {
                    parameterCountStack.push(0);
                    if (!parameterExistsStack.empty())
                    {
                        parameterExistsStack.pop();
                        parameterExistsStack.push(true);
                    }

                    parameterExistsStack.push(false);
                }

                stack.push(token);
                break;

            case TokenType::RightParenthesis:
                if (stack.empty())
                {
                    throw UnbalancedParenthesis("Unmatched right parenthesis found");
                }

                while (stack.top().type != TokenType::LeftParenthesis)
                {
                    rpnTokenList.push_back(stack.popTop());
                    if (stack.empty())
                    {
                        throw UnbalancedParenthesis("Unmatched right parenthesis found");
                    }
                };

                stack.pop();
                if (stack.empty() && stack.top().type == TokenType::Function)
                {
                    Token function = stack.popTop();
                    function.parameterCount = parameterCountStack.popTop();
                    if (parameterExistsStack.popTop())
                    {
                        function.parameterCount++;
                    }

                    rpnTokenList.push_back(function);
                }

                break;

            case TokenType::Separator:
                if (stack.empty())
                {
                    throw MisplacedSeparator("Separator encountered outside of parenthesis block");
                }

                if (parameterExistsStack.empty())
                {
                    throw MisplacedSeparator("Separator encountered at start of parenthesis block");
                }

                while (stack.top().type != TokenType::LeftParenthesis)
                {
                    rpnTokenList.push_back(stack.popTop());
                    if (stack.empty())
                    {
                        throw MisplacedSeparator("Separator encountered without leading left parenthesis");
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
                stack.push(token);
                break;

            default:
                throw UnknownTokenType("Unknown token type encountered");
            };
        }

        while (!stack.empty())
        {
            if (stack.top().type == TokenType::LeftParenthesis || stack.top().type == TokenType::RightParenthesis)
            {
                throw UnbalancedParenthesis("Invalid surrounding parenthesis encountered");
            }

            rpnTokenList.push_back(stack.popTop());
        };

        if (rpnTokenList.empty())
        {
            throw InvalidEquation("Empty equation encountered");
        }

        return rpnTokenList;
    }

    float ShuntingYard::evaluateReversePolishNotation(const TokenList &rpnTokenList)
    {
        Stack<Token> stack;
        for (auto &token : rpnTokenList)
        {
            switch (token.type)
            {
            case TokenType::Number:
                stack.push(token);
                break;

            case TokenType::UnaryOperation:
                if (true)
                {
                    auto &operationSearch = operationsMap.find(token.string);
                    if (operationSearch == std::end(operationsMap))
                    {
                        throw InvalidOperator("Unlisted unary operation encountered");
                    }

                    auto &operation = operationSearch->second;
                    if (!operation.unaryFunction)
                    {
                        throw InvalidOperator("Missing unary function encountered");
                    }

                    if (stack.empty())
                    {
                        throw InvalidOperand("Unary function encountered without parameter");
                    }

                    if (stack.top().type != TokenType::Number)
                    {
                        throw InvalidOperand("Unary function requires numeric parameter");
                    }

                    float functionValue = stack.popTop().value;

                    stack.push(Token(operation.unaryFunction(functionValue)));
                    break;
                }

            case TokenType::BinaryOperation:
                if (true)
                {
                    auto &operationSearch = operationsMap.find(token.string);
                    if (operationSearch == std::end(operationsMap))
                    {
                        throw InvalidOperator("Unlisted binary operation encounterd");
                    }

                    auto &operation = operationSearch->second;
                    if (!operation.binaryFunction)
                    {
                        throw InvalidOperator("Missing binary function encountered");
                    }

                    if (stack.empty())
                    {
                        throw InvalidOperand("Binary function used without first parameter");
                    }

                    if (stack.top().type != TokenType::Number)
                    {
                        throw InvalidOperand("Binary function requires numeric first parameter");
                    }

                    float functionValueRight = stack.popTop().value;
                    if (stack.empty())
                    {
                        throw InvalidOperand("Binary function used without second parameter");
                    }

                    if (stack.top().type != TokenType::Number)
                    {
                        throw InvalidOperand("Binary function requires numeric second parameter");
                    }

                    float functionValueLeft = stack.popTop().value;
                    stack.push(Token(operation.binaryFunction(functionValueLeft, functionValueRight)));
                    break;
                }

            case TokenType::Function:
                if (true)
                {
                    auto &functionSearch = functionsMap.find(token.string);
                    if (functionSearch == std::end(functionsMap))
                    {
                        throw InvalidFunction("Unlisted function encountered");
                    }

                    auto &function = functionSearch->second;
                    if (function.parameterCount != token.parameterCount)
                    {
                        throw InvalidFunctionParameters("Expected different number of parameters for function");
                    }

                    if (stack.size() < function.parameterCount)
                    {
                        throw NotEnoughFunctionParameters("Not enough parameters passed to function");
                    }

                    stack.push(Token(function.function(stack)));
                    break;
                }

            default:
                throw UnknownTokenType("Unknown token type encountered");
            };
        }

        if (rpnTokenList.empty())
        {
            throw InvalidEquation("Empty equation encountered");
        }

        if (stack.size() != 1)
        {
            throw InvalidEquation("Too many values left in stack");
        }

        return stack.popTop().value;
    }
}; // namespace Gek
