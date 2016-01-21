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

template <typename TYPE>
class ShuntingYard
{
public:
    enum class Associations : UINT8
    {
        Left = 0,
        Right,
    };

    struct Operation
    {
        int precedence;
        Associations association;
    };

    static const std::map<CStringW, Operation> operationMap;
    static const std::set<CStringW> functionList;

    bool isOperator(const CStringW &token)
    {
        return (operationMap.count(token) > 0);
    }

    bool isParenthesis(const CStringW &token)
    {
        return token == L"(" || token == L")";
    }

    bool isSeparator(const CStringW &token)
    {
        return token == L",";
    }

    std::vector<CStringW> convertExpressionToInfix(const CStringW &expression)
    {
        CStringW string;
        std::vector<CStringW> infixTokenList;
        for (int index = 0; index < expression.GetLength(); ++index)
        {
            const CStringW token(expression.GetAt(index));

            if (isOperator(token) || isParenthesis(token) || isSeparator(token))
            {
                if (!string.IsEmpty())
                {
                    infixTokenList.push_back(string);
                }

                string.Empty();
                infixTokenList.push_back(token);
            }
            else
            {
                // Append the numbers      
                if (!token.IsEmpty() && token != " ")
                {
                    string.Append(token);
                }
                else
                {
                    if (!string.IsEmpty())
                    {
                        infixTokenList.push_back(string);
                        string.Empty();
                    }
                }
            }
        }

        if (!string.IsEmpty())
        {
            infixTokenList.push_back(string);
        }

        return infixTokenList;
    }

    bool isAssociative(const CStringW &token, const Associations& type)
    {
        auto &p = operationMap.find(token)->second;
        return p.association == type;
    }

    int comparePrecedence(const CStringW &token1, const CStringW &token2)
    {
        auto &p1 = operationMap.find(token1)->second;
        auto &p2 = operationMap.find(token2)->second;
        return p1.precedence - p2.precedence;
    }

    bool isNumber(const CStringW &token)
    {
        return std::regex_match(token.GetString(), std::wregex(L"^(\\-|\\+)?[0-9]*(\\.[0-9]+)?"));
    }

    bool isFunction(const CStringW &token)
    {
        return functionList.count(token) > 0;
    }

    bool convertInfixToReversePolishNotation(const std::vector<CStringW> &infixTokenList, std::vector<CStringW> &rpnTokenList)
    {
        std::stack<CStringW> stack;
        int size = infixTokenList.size();
        for (auto &token : infixTokenList)
        {
            if (isOperator(token))
            {
                while (!stack.empty() && isOperator(stack.top()) &&
                    (isAssociative(token, Associations::Left) && comparePrecedence(token, stack.top()) == 0) ||
                    (isAssociative(token, Associations::Right) && comparePrecedence(token, stack.top()) < 0))
                {
                    rpnTokenList.push_back(stack.top());
                    stack.pop();
                };

                stack.push(token);
            }
            else if (isFunction(token))
            {
                stack.push(token);
            }
            else if (isSeparator(token))
            {
                if (stack.empty())
                {
                    // we can't not have values on the stack and find a parameter separator
                    return false;
                }

                while (stack.top() != L"(")
                {
                    rpnTokenList.push_back(stack.top());
                    stack.pop();

                    if (stack.empty())
                    {
                        // found a separator with a starting parenthesis
                        return false;
                    }
                };
            }
            else if (token == L"(")
            {
                stack.push(token);
            }
            else if (token == L")")
            {
                if (stack.empty())
                {
                    // we can't have an ending parenthesis without a starting one
                    return false;
                }

                while (stack.top() != L"(")
                {
                    rpnTokenList.push_back(stack.top());
                    stack.pop();

                    if (stack.empty())
                    {
                        // found a closing parenthesis without a starting one
                        return false;
                    }
                };

                stack.pop();
                if (!stack.empty() && isFunction(stack.top()))
                {
                    rpnTokenList.push_back(stack.top());
                    stack.pop();
                }
            }
            else if (isNumber(token))
            {
                rpnTokenList.push_back(token);
            }
            else
            {
                // what's in the box?!?
                return false;
            }
        }

        while (!stack.empty())
        {
            if (isParenthesis(stack.top()))
            {
                // all parenthesis should have been handled above
                return false;
            }

            rpnTokenList.push_back(stack.top());
            stack.pop();
        };

        return true;
    }

    bool evaluateReversePolishNotation(const std::vector<CStringW> &rpnTokenList, TYPE &value)
    {
        std::stack<CStringW> stack;
        for (auto &token : rpnTokenList)
        {
            if (!isOperator(token) && isNumber(token))
            {
                stack.push(token);
            }
            else if (isOperator(token))
            {
                if (token == L"+")
                {
                    if (stack.empty())
                    {
                        return false;
                    }

                    TYPE value2 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    if (!stack.empty() && !isOperator(stack.top()) && isNumber(stack.top()))
                    {
                        TYPE value1 = Gek::String::to<TYPE>(stack.top());
                        stack.pop();

                        stack.push(Gek::String::from(value1 + value2));
                    }
                    else
                    {
                        stack.push(Gek::String::from(value2));
                    }
                }
                else if (token == L"-")
                {
                    if (stack.empty())
                    {
                        return false;
                    }

                    TYPE value2 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    if (!stack.empty() && !isOperator(stack.top()) && isNumber(stack.top()))
                    {
                        TYPE value1 = Gek::String::to<TYPE>(stack.top());
                        stack.pop();

                        stack.push(Gek::String::from(value1 - value2));
                    }
                    else
                    {
                        stack.push(Gek::String::from(-value2));
                    }
                }
                else if (token == L"*")
                {
                    if (stack.size() < 2)
                    {
                        return false;
                    }

                    TYPE value2 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    TYPE value1 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    stack.push(Gek::String::from(value1 * value2));
                }
                else if (token == L"/")
                {
                    if (stack.size() < 2)
                    {
                        return false;
                    }

                    TYPE value2 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    TYPE value1 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    stack.push(Gek::String::from(value1 / value2));
                }
                else if (token == L"^")
                {
                    if (stack.size() < 2)
                    {
                        return false;
                    }

                    TYPE value2 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    TYPE value1 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    stack.push(Gek::String::from(std::pow(value1, value2)));
                }
            }
            else if (isFunction(token))
            {
                if (token == L"sin")
                {
                    if (stack.empty())
                    {
                        return false;
                    }

                    TYPE value = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    stack.push(Gek::String::from(std::sin(value)));
                }
                else if (token == L"cos")
                {
                    if (stack.empty())
                    {
                        return false;
                    }

                    TYPE value = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    stack.push(Gek::String::from(std::cos(value)));
                }
                else if (token == L"tan")
                {
                    if (stack.empty())
                    {
                        return false;
                    }

                    TYPE value = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    stack.push(Gek::String::from(std::tan(value)));
                }
                else if (token == L"min")
                {
                    if (stack.size() < 2)
                    {
                        return false;
                    }

                    TYPE value2 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    TYPE value1 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    stack.push(Gek::String::from(std::min(value1, value2)));
                }
                else if (token == L"max")
                {
                    if (stack.size() < 2)
                    {
                        return false;
                    }

                    TYPE value2 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    TYPE value1 = Gek::String::to<TYPE>(stack.top());
                    stack.pop();

                    stack.push(Gek::String::from(std::max(value1, value2)));
                }
            }
            else
            {
                return false;
            }
        }

        if (stack.size() == 1)
        {
            value = Gek::String::to<TYPE>(stack.top());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool evaluate(const CStringW &expression, TYPE &value)
    {
        auto infixTokenList = convertExpressionToInfix(expression);

        std::vector<CStringW> rpnTokenList;
        if (convertInfixToReversePolishNotation(infixTokenList, rpnTokenList))
        {
            return evaluateReversePolishNotation(rpnTokenList, value);
        }

        return false;
    }
};

template <typename TYPE>
const std::map<CStringW, typename ShuntingYard<TYPE>::Operation> ShuntingYard<TYPE>::operationMap =
{
    { L"^",{ 4, Associations::Right } },
    { L"*",{ 3, Associations::Left } },
    { L"/",{ 3, Associations::Left } },
    { L"+",{ 2, Associations::Left } },
    { L"-",{ 2, Associations::Left } },
};

template <typename TYPE>
const std::set<CStringW> ShuntingYard<TYPE>::functionList =
{
    L"sin",
    L"cos",
    L"tan",
    L"min",
    L"max",
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

    LPCWSTR expressionList[] =
    {
        L"3 + 4 * 2 / ( 1 - 5 ) ^ 2 ^ 3",
        L"sin ( max ( 2 , 3 ) / 3 * 3.1415 )",
    };

    ShuntingYard<float> shuntingYardFloat;
    for (auto &expression : expressionList)
    {
        float value = 0.0f;
        if (shuntingYardFloat.evaluate(expression, value))
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