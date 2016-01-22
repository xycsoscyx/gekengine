#include <initguid.h>
#include <cguid.h>

#include "GEK\Utility\Display.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\Context.h"
#include "GEK\Engine\Engine.h"
#include <CommCtrl.h>
#include "resource.h"
#include <regex>
#include <stack>
#include <set>

INT_PTR CALLBACK DialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(dialog, IDCANCEL);
        return TRUE;

    case WM_INITDIALOG:
    {
        UINT32 width = 800;
        UINT32 height = 600;
        bool windowed = true;

        Gek::XmlDocument xmlDocument;
        if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
        {
            Gek::XmlNode xmlConfigNode = xmlDocument.getRoot();
            if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
            {
                Gek::XmlNode xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                if (xmlDisplayNode)
                {
                    if (xmlDisplayNode.hasAttribute(L"width"))
                    {
                        width = Gek::String::to<UINT32>(xmlDisplayNode.getAttribute(L"width"));
                    }

                    if (xmlDisplayNode.hasAttribute(L"height"))
                    {
                        height = Gek::String::to<UINT32>(xmlDisplayNode.getAttribute(L"height"));
                    }

                    if (xmlDisplayNode.hasAttribute(L"windowed"))
                    {
                        windowed = Gek::String::to<bool>(xmlDisplayNode.getAttribute(L"windowed"));
                    }
                }
            }
        }

        UINT32 selectIndex = 0;
        SendDlgItemMessage(dialog, IDC_MODES, CB_RESETCONTENT, 0, 0);
        std::vector<Gek::DisplayMode> modeList = Gek::getDisplayModes()[32];
        for (auto &mode : modeList)
        {
            CStringW aspectRatio(L"");
            switch (mode.aspectRatio)
            {
            case Gek::AspectRatio::_4x3:
                aspectRatio = L", (4x3)";
                break;

            case Gek::AspectRatio::_16x9:
                aspectRatio = L", (16x9)";
                break;

            case Gek::AspectRatio::_16x10:
                aspectRatio = L", (16x10)";
                break;
            };

            CStringW modeString;
            modeString.Format(L"%dx%d%s", mode.width, mode.height, aspectRatio.GetString());
            int modeIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_ADDSTRING, 0, (WPARAM)modeString.GetString());
            if (mode.width == width && mode.height == height)
            {
                selectIndex = modeIndex;
            }
        }

        SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_SETCHECK, windowed ? BST_UNCHECKED : BST_CHECKED, 0);

        SendDlgItemMessage(dialog, IDC_MODES, CB_SETMINVISIBLE, 5, 0);
        SendDlgItemMessage(dialog, IDC_MODES, CB_SETEXTENDEDUI, TRUE, 0);
        SendDlgItemMessage(dialog, IDC_MODES, CB_SETCURSEL, selectIndex, 0);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            std::vector<Gek::DisplayMode> modeList = Gek::getDisplayModes()[32];
            UINT32 selectIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_GETCURSEL, 0, 0);
            auto &mode = modeList[selectIndex];

            Gek::XmlDocument xmlDocument;
            xmlDocument.load(L"%root%\\config.xml");
            Gek::XmlNode xmlConfigNode = xmlDocument.getRoot();
            if (!xmlConfigNode || xmlConfigNode.getType().CompareNoCase(L"config") != 0)
            {
                xmlDocument.create(L"config");
                xmlConfigNode = xmlDocument.getRoot();
            }

            Gek::XmlNode xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
            if (!xmlDisplayNode)
            {
                xmlDisplayNode = xmlConfigNode.createChildElement(L"display");
            }

            xmlDisplayNode.setAttribute(L"width", L"%d", mode.width);
            xmlDisplayNode.setAttribute(L"height", L"%d", mode.height);
            xmlDisplayNode.setAttribute(L"windowed", L"%s", (SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_GETCHECK, 0, 0) == BST_UNCHECKED ? L"true" : L"false"));
            xmlDocument.save(L"%root%\\config.xml");

            EndDialog(dialog, IDOK);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        };

        return TRUE;
    };

    return FALSE;
}

LRESULT CALLBACK WindowProc(HWND window, UINT32 message, WPARAM wParam, LPARAM lParam)
{
    LRESULT resultValue = 0;
    Gek::Engine *engineCore = (Gek::Engine *)GetWindowLongPtr(window, GWLP_USERDATA);
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        if (engineCore)
        {
            resultValue = engineCore->windowEvent(message, wParam, lParam);
        }

        break;
    };

    if (resultValue == 0)
    {
        resultValue = DefWindowProc(window, message, wParam, lParam);
    }

    return resultValue;
}

// https://blog.kallisti.net.nz/2008/02/extension-to-the-shunting-yard-algorithm-to-allow-variable-numbers-of-arguments-to-functions/
template <typename TYPE>
class ShuntingYard
{
private:
    enum class Associations : UINT8
    {
        Left = 0,
        Right,
    };

    enum class TokenType : UINT8
    {
        Unknown = 0,
        Number,
        Operation,
        LeftParenthesis,
        RightParenthesis,
        Separator,
        Function,
    };

    struct Token
    {
        TokenType type;
        CStringW value;
        UINT32 parameterCount;

        Token(void)
        {
        }

        Token(TokenType type, const CStringW &value, UINT32 parameterCount = 0)
            : type(type)
            , value(value)
            , parameterCount(parameterCount)
        {
        }

        operator const LPCWSTR() const
        {
            return value.GetString();
        }

        operator const CStringW &() const
        {
            return value;
        }

        bool operator == (const Token &token) const
        {
            return value == token.value;
        }

        bool operator == (LPCWSTR value) const
        {
            return this->value.Compare(value) == 0;
        }
    };

    template <typename DATA>
    struct Stack : public std::stack<DATA>
    {
        DATA popTop(void)
        {
            DATA topElement = top();
            stack::pop();
            return topElement;
        }

        TYPE popValue(void)
        {
            DATA topElement = top();
            stack::pop();
            return Gek::String::to<TYPE>(topElement);
        }
    };

    struct Operation
    {
        int precedence;
        Associations association;
        std::function<TYPE(TYPE value)> unaryFunction;
        std::function<TYPE(TYPE valueLeft, TYPE valueRight)> binaryFunction;
    };

    struct Function
    {
        UINT32 parameterCount;
        std::function<TYPE(Stack<Token> &)> function;
    };

private:
    std::map<CStringW, CStringW> variableMap;
    std::map<CStringW, Operation> operationsMap;
    std::map<CStringW, Function> functionsMap;
    std::locale locale;

public:
    enum class Status : UINT8
    {
        Success = 0,
        UnknownTokenType,
        UnbalancedEquation,
        UnbalancedParenthesis,
        InvalidOperator,
        InvalidFunctionParameters,
        MisplacedSeparator,
        MissingFunctionParenthesis,
    };

public:
    ShuntingYard(void)
        : locale("")
    {
        variableMap[L"pi"] = L"3.14159265358979323846";
        variableMap[L"e"] = L"2.71828182845904523536";

        operationsMap.insert({ L"^",{ 4, Associations::Right, nullptr, [](TYPE valueLeft, TYPE valueRight) -> TYPE
        {
            return std::pow(valueLeft, valueRight);
        } } });

        operationsMap.insert({ L"*",{ 3, Associations::Left, nullptr, [](TYPE valueLeft, TYPE valueRight) -> TYPE
        {
            return (valueLeft * valueRight);
        } } });

        operationsMap.insert({ L"/",{ 3, Associations::Left, nullptr, [](TYPE valueLeft, TYPE valueRight) -> TYPE
        {
            return (valueLeft / valueRight);
        } } });

        operationsMap.insert({ L"+",{ 2, Associations::Left, [](TYPE value) -> TYPE
        {
            return value;
        }, [](TYPE valueLeft, TYPE valueRight) -> TYPE
        {
            return (valueLeft + valueRight);
        } } });

        operationsMap.insert({ L"-",{ 2, Associations::Left, [](TYPE value) -> TYPE
        {
            return -value;
        }, [](TYPE valueLeft, TYPE valueRight) -> TYPE
        {
            return (valueLeft - valueRight);
        } } });

        functionsMap.insert({ L"sin",{ 1, [](Stack<Token> &stack) -> TYPE
        {
            TYPE value = stack.popValue();
            return std::sin(value);
        } } });

        functionsMap.insert({ L"cos",{ 1, [](Stack<Token> &stack) -> TYPE
        {
            TYPE value = stack.popValue();
            return std::cos(value);
        } } });

        functionsMap.insert({ L"tan",{ 1, [](Stack<Token> &stack) -> TYPE
        {
            TYPE value = stack.popValue();
            return std::tan(value);
        } } });

        functionsMap.insert({ L"asin",{ 1, [](Stack<Token> &stack) -> TYPE
        {
            TYPE value = stack.popValue();
            return std::asin(value);
        } } });

        functionsMap.insert({ L"acos",{ 1, [](Stack<Token> &stack) -> TYPE
        {
            TYPE value = stack.popValue();
            return std::acos(value);
        } } });

        functionsMap.insert({ L"atan",{ 1, [](Stack<Token> &stack) -> TYPE
        {
            TYPE value = stack.popValue();
            return std::atan(value);
        } } });

        functionsMap.insert({ L"min",{ 2, [](Stack<Token> &stack) -> TYPE
        {
            TYPE value2 = stack.popValue();
            TYPE value1 = stack.popValue();
            return std::min(value1, value2);
        } } });

        functionsMap.insert({ L"max",{ 2, [](Stack<Token> &stack) -> TYPE
        {
            TYPE value2 = stack.popValue();
            TYPE value1 = stack.popValue();
            return std::max(value1, value2);
        } } });
    }

    Status evaluate(const CStringW &expression, TYPE &value)
    {
        std::vector<Token> infixTokenList = convertExpressionToInfix(expression);

        std::vector<Token> rpnTokenList;
        Status status = convertInfixToReversePolishNotation(infixTokenList, rpnTokenList);
        if (status == Status::Success)
        {
            status = evaluateReversePolishNotation(rpnTokenList, value);
        }

        return status;
    }

private:
    bool isNumber(const CStringW &token)
    {
        return std::regex_match(token.GetString(), std::wregex(L"^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$"));
    }

    bool isOperation(const CStringW &token)
    {
        return (operationsMap.count(token) > 0);
    }

    bool isFunction(const CStringW &token)
    {
        return (functionsMap.count(token) > 0);
    }

    bool isLeftParenthesis(const CStringW &token)
    {
        return token == L'(';
    }

    bool isRightParenthesis(const CStringW &token)
    {
        return token == L')';
    }

    bool isParenthesis(const CStringW &token)
    {
        return (isLeftParenthesis(token) || isRightParenthesis(token));
    }

    bool isSeparator(const CStringW &token)
    {
        return token == L',';
    }

    bool isAssociative(const CStringW &token, const Associations &type)
    {
        auto &p = operationsMap.find(token)->second;
        return p.association == type;
    }

    int comparePrecedence(const CStringW &token1, const CStringW &token2)
    {
        auto &p1 = operationsMap.find(token1)->second;
        auto &p2 = operationsMap.find(token2)->second;
        return p1.precedence - p2.precedence;
    }

    TokenType getTokenType(const CStringW &token)
    {
        if (isNumber(token))
        {
            return TokenType::Number;
        }
        else if (isOperation(token))
        {
            return TokenType::Operation;
        }
        else if (isFunction(token))
        {
            return TokenType::Function;
        }
        else if (isSeparator(token))
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

        return TokenType::Unknown;
    }

private:
    void checkForImplicitMultiplication(std::vector<Token> &infixTokenList, const Token &token)
    {
        if (!infixTokenList.empty())
        {
            const Token &previous = infixTokenList.back();

            // ) 2 or 2 2
            if (token.type == TokenType::Number && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::Operation, L"*"));
            }
            // 2 ( or ) (
            else if (token.type == TokenType::LeftParenthesis && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::Operation, L"*"));
            }
            // ) sin or 2 sin
            else if (token.type == TokenType::Function && (previous.type == TokenType::Number || previous.type == TokenType::RightParenthesis))
            {
                infixTokenList.push_back(Token(TokenType::Operation, L"*"));
            }
        }

        infixTokenList.push_back(token);
    }

    bool replaceFirstVariable(std::vector<Token> &infixTokenList, CStringW &token)
    {
        for (auto &variable : variableMap)
        {
            if (token.Find(variable.first) == 0)
            {
                checkForImplicitMultiplication(infixTokenList, Token(TokenType::Number, variable.second));
                token = token.Mid(variable.first.GetLength());
                return true;
            }
        }

        return false;
    }

    bool replaceFirstFunction(std::vector<Token> &infixTokenList, CStringW &token)
    {
        for (auto &function : functionsMap)
        {
            if (token.Find(function.first) == 0)
            {
                checkForImplicitMultiplication(infixTokenList, Token(TokenType::Function, function.first));
                token = token.Mid(function.first.GetLength());
                return true;
            }
        }

        return false;
    }

    Status parseSubTokens(std::vector<Token> &infixTokenList, CStringW token)
    {
        while (!token.IsEmpty())
        {
            std::wcmatch matches;
            if (std::regex_search(token.GetString(), matches, std::wregex(L"^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?")))
            {
                CStringW value = matches[0].str().c_str();
                checkForImplicitMultiplication(infixTokenList, Token(TokenType::Number, value));
                token = token.Mid(value.GetLength());
                continue;
            }

            if (replaceFirstVariable(infixTokenList, token))
            {
                continue;
            }

            if (replaceFirstFunction(infixTokenList, token))
            {
                if (!token.IsEmpty())
                {
                    // function must be followed by a left parenthesis, so it has to be at the end of an implicit block
                    return Status::MissingFunctionParenthesis;
                }

                continue;
            }

            if (!token.IsEmpty())
            {
                // nothing was replaced, yet we still have remaining characters?
                return Status::UnknownTokenType;
            }
        };

        return Status::Success;
    }

    std::vector<Token> convertExpressionToInfix(const CStringW &expression)
    {
        CStringW runningToken;
        std::vector<Token> infixTokenList;
        for (int index = 0; index < expression.GetLength(); ++index)
        {
            CStringW nextToken = expression.GetAt(index);
            if (isOperation(nextToken) || isParenthesis(nextToken) || isSeparator(nextToken))
            {
                if (!runningToken.IsEmpty())
                {
                    parseSubTokens(infixTokenList, runningToken);
                    runningToken.Empty();
                }

                checkForImplicitMultiplication(infixTokenList, Token(getTokenType(nextToken), nextToken));
            }
            else
            {
                if (nextToken == L" ")
                {
                    if (!runningToken.IsEmpty())
                    {
                        parseSubTokens(infixTokenList, runningToken);
                        runningToken.Empty();
                    }
                }
                else
                {
                    runningToken.Append(nextToken);
                }
            }
        }

        if (!runningToken.IsEmpty())
        {
            parseSubTokens(infixTokenList, runningToken);
        }

        return infixTokenList;
    }

    Status convertInfixToReversePolishNotation(const std::vector<Token> &infixTokenList, std::vector<Token> &rpnTokenList)
    {
        Stack<Token> stack;
        Stack<bool> parameterExistsStack;
        Stack<UINT32> parameterCountStack;
        int size = infixTokenList.size();
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

            case TokenType::Operation:
                while (!stack.empty() && stack.top().type == TokenType::Operation &&
                    (isAssociative(token, Associations::Left) && comparePrecedence(token, stack.top()) == 0) ||
                    (isAssociative(token, Associations::Right) && comparePrecedence(token, stack.top()) < 0))
                {
                    rpnTokenList.push_back(stack.popTop());
                };

                stack.push(token);
                break;

            case TokenType::LeftParenthesis:
                 stack.push(token);
                 break;

            case TokenType::RightParenthesis:
                if (stack.empty())
                {
                    // we can't have an ending parenthesis without a starting one
                    return Status::UnbalancedParenthesis;
                }

                while (stack.top().type != TokenType::LeftParenthesis)
                {
                    rpnTokenList.push_back(stack.popTop());
                    if (stack.empty())
                    {
                        // found a closing parenthesis without a starting one
                        return Status::UnbalancedParenthesis;
                    }
                };

                stack.pop();
                if (!stack.empty())
                {
                    if (isFunction(stack.top()))
                    {
                        UINT32 parameterCount = parameterCountStack.popTop();
                        if (parameterExistsStack.popTop())
                        {
                            parameterCount++;
                        }

                        rpnTokenList.push_back(Token(TokenType::Function, stack.popTop(), parameterCount));
                    }
                }
                
                break;

            case TokenType::Separator:
                if (stack.empty())
                {
                    // we can't not have values on the stack and find a parameter separator
                    return Status::MisplacedSeparator;
                }

                while (stack.top().type != TokenType::LeftParenthesis)
                {
                    rpnTokenList.push_back(stack.popTop());
                    if (stack.empty())
                    {
                        // found a separator with a starting parenthesis
                        return Status::MisplacedSeparator;
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
                parameterCountStack.push(0);
                if (!parameterExistsStack.empty())
                {
                    parameterExistsStack.pop();
                    parameterExistsStack.push(true);
                }

                parameterExistsStack.push(false);
                break;

            default:
                return Status::UnknownTokenType;
            };
        }

        while (!stack.empty())
        {
            if (stack.top().type == TokenType::LeftParenthesis ||
                stack.top().type == TokenType::RightParenthesis)
            {
                // all parenthesis should have been handled above
                return Status::UnbalancedParenthesis;
            }

            CStringW token = stack.popTop();
            rpnTokenList.push_back(Token(getTokenType(token), token));
        };

        return Status::Success;
    }

    Status evaluateReversePolishNotation(const std::vector<Token> &rpnTokenList, TYPE &value)
    {
        Stack<Token> stack;
        for (auto &token : rpnTokenList)
        {
            switch (token.type)
            {
            case TokenType::Number:
                stack.push(token);
                break;

            case TokenType::Operation:
                if (true)
                {
                    auto &operation = operationsMap.find(token)->second;

                    union
                    {
                        TYPE value;
                        TYPE valueRight;
                    };

                    value = stack.popValue();
                    if (operation.binaryFunction && !stack.empty() && !isOperation(stack.top()) && isNumber(stack.top()))
                    {
                        TYPE valueLeft = stack.popValue();
                        stack.push(Token(TokenType::Number, Gek::String::from(operation.binaryFunction(valueLeft, valueRight))));
                    }
                    else if (operation.unaryFunction)
                    {
                        stack.push(Token(TokenType::Number, Gek::String::from(operation.unaryFunction(value))));
                    }
                    else
                    {
                        return Status::InvalidOperator;
                    }

                    break;
                }

            case TokenType::Function:
                if (true)
                {
                    auto &function = functionsMap.find(token)->second;
                    if (function.parameterCount != token.parameterCount)
                    {
                        return Status::InvalidFunctionParameters;
                    }

                    if (stack.size() < function.parameterCount)
                    {
                        return Status::InvalidFunctionParameters;
                    }

                    stack.push(Token(TokenType::Number, Gek::String::from(function.function(stack))));
                    break;
                }

            default:
                return Status::UnknownTokenType;
            };
        }

        if (stack.size() == 1)
        {
            value = Gek::String::to<TYPE>(stack.top());
            return Status::Success;
        }
        else
        {
            return Status::UnbalancedEquation;
        }
    }
};

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR strCommandLine, _In_ int nCmdShow)
{
    static const LPCWSTR materialFormat = \
        L"<?xml version=\"1.0\"?>\r\n" \
        L"<material>\r\n" \
        L"    <shader>standard</shader>\r\n" \
        L"    <maps>\r\n" \
        L"        <albedo>schulz/old_tiles/old_tiles_a</albedo>\r\n" \
        L"        <normal>schulz/old_tiles/old_tiles_n</normal>\r\n" \
        L"        <roughness>*color:%f</roughness>\r\n" \
        L"        <metalness>*color:%f</metalness>\r\n" \
        L"    </maps>\r\n" \
        L"</material>";

    static const LPCWSTR entityFormat = \
        L"        <entity>\r\n" \
        L"            <transform position=\"0,%d,%d\" scale=\".5,.5,.5\" />\r\n" \
        /*L"            <color>lerp(.5,1,arand(1)),lerp(.5,1,arand(1)),lerp(.5,1,arand(1)),1</color>\r\n" \*/
        L"            <model>*sphere|debug//r%d_m%d|1</model>\r\n" \
        L"        </entity>\r\n";

    CStringW entities;
    for (UINT32 roughness = 0; roughness < 11; roughness++)
    {
        for (UINT32 metalness = 0; metalness < 11; metalness++)
        {
            CStringW material(Gek::String::format(materialFormat, (float(roughness) / 10.0f), (float(metalness) / 10.0f)));
            CStringW fileName(Gek::String::format(L"r%d_m%d.xml", roughness, metalness));
            Gek::FileSystem::save((L"%root%\\data\\materials\\debug\\" + fileName), material);

            entities += Gek::String::format(entityFormat, (roughness - 5) + 10, (metalness - 5), roughness, metalness);
        }
    }

    static const CStringW expressionList[] =
    {
        L"2pie3",
        L"1(2(3(4+5)))",
        L"(2)(3)",
        L"3 + 4 * 2 / ( 1 - 5 ) ^ 2 ^ 3",
        L"sin ( max ( 2 , 3 ) / 3 * 3.1415 )",
        L"sin(max(2,3)/3*3.1415)",
        L"   sin(max(2,3)/3*3.1415)",
        L"   sin(max(2,3)/3*3.1415)   ",
        L"sin(max(2,3)/3*3.1415)   ",
    };

    ShuntingYard<float> shuntingYard;
    for (auto &expression : expressionList)
    {
        float value = 0.0f;
        if (shuntingYard.evaluate(expression, value) == ShuntingYard<float>::Status::Success)
        {
            OutputDebugString(L"yay");
        }
    }

    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, DialogProc) == IDOK)
    {
        //_clearfp();
        //unsigned unused_current_word = 0;
        //_controlfp_s(&unused_current_word, 0, _EM_ZERODIVIDE | _EM_INVALID);

        CComPtr<Gek::Context> context;
        Gek::Context::create(&context);
        if (context)
        {
#ifdef _DEBUG
            SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Debug"));
            context->addSearchPath(L"%root%\\Debug\\Plugins");
#else
            SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Release"));
            context->addSearchPath(L"%root%\\Release\\Plugins");
#endif

            context->initialize();
            CComPtr<Gek::Engine> engineCore;
            context->createInstance(CLSID_IID_PPV_ARGS(Gek::EngineRegistration, &engineCore));
            if (engineCore)
            {
                WNDCLASS kClass;
                kClass.style = 0;
                kClass.lpfnWndProc = WindowProc;
                kClass.cbClsExtra = 0;
                kClass.cbWndExtra = 0;
                kClass.hInstance = GetModuleHandle(nullptr);
                kClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
                kClass.hCursor = nullptr;
                kClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
                kClass.lpszMenuName = nullptr;
                kClass.lpszClassName = L"GEKvX_Engine_Demo";
                if (RegisterClass(&kClass))
                {
                    UINT32 width = 800;
                    UINT32 height = 600;
                    bool windowed = true;

                    Gek::XmlDocument xmlDocument;
                    if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
                    {
                        Gek::XmlNode xmlConfigNode = xmlDocument.getRoot();
                        if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
                        {
                            Gek::XmlNode xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                            if (xmlDisplayNode)
                            {
                                if (xmlDisplayNode.hasAttribute(L"width"))
                                {
                                    width = Gek::String::to<UINT32>(xmlDisplayNode.getAttribute(L"width"));
                                }

                                if (xmlDisplayNode.hasAttribute(L"height"))
                                {
                                    height = Gek::String::to<UINT32>(xmlDisplayNode.getAttribute(L"height"));
                                }

                                if (xmlDisplayNode.hasAttribute(L"windowed"))
                                {
                                    windowed = Gek::String::to<bool>(xmlDisplayNode.getAttribute(L"windowed"));
                                }
                            }
                        }
                    }

                    RECT clientRect;
                    clientRect.left = 0;
                    clientRect.top = 0;
                    clientRect.right = width;
                    clientRect.bottom = height;
                    AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, false);
                    int windowWidth = (clientRect.right - clientRect.left);
                    int windowHeight = (clientRect.bottom - clientRect.top);
                    int centerPositionX = (windowed ? (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - ((clientRect.right - clientRect.left) / 2) : 0);
                    int centerPositionY = (windowed ? (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - ((clientRect.bottom - clientRect.top) / 2) : 0);
                    HWND window = CreateWindow(L"GEKvX_Engine_Demo", L"GEKvX Engine - Demo", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, centerPositionX, centerPositionY, windowWidth, windowHeight, 0, nullptr, GetModuleHandle(nullptr), 0);
                    if (window)
                    {
                        if (SUCCEEDED(engineCore->initialize(window)))
                        {
                            SetWindowLongPtr(window, GWLP_USERDATA, LONG((Gek::Engine *)engineCore));
                            ShowWindow(window, SW_SHOW);
                            UpdateWindow(window);

                            context->logMessage(__FILE__, __LINE__, 1, L"[entering] Game Loop");

                            MSG message = { 0 };
                            while (message.message != WM_QUIT)
                            {
                                while (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
                                {
                                    TranslateMessage(&message);
                                    DispatchMessage(&message);
                                };

                                if (!engineCore->update())
                                {
                                    break;
                                }
                            };

                            context->logMessage(__FILE__, __LINE__, -1, L"[entering] Game Loop");

                            SetWindowLongPtr(window, GWLP_USERDATA, 0);
                            engineCore.Release();
                            DestroyWindow(window);
                        }
                    }
                    else
                    {
                        context->logMessage(__FILE__, __LINE__, 0, L"Unable to create window: %d", GetLastError());
                    }
                }
                else
                {
                    context->logMessage(__FILE__, __LINE__, 0, L"Unable to register window class: %d", GetLastError());
                }
            }
        }

        return 0;
    }

    return -1;
}