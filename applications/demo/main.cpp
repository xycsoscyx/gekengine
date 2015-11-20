#include <initguid.h>
#include <cguid.h>

#include "GEK\Utility\Display.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\Interface.h"
#include "GEK\Engine\CoreInterface.h"
#include <CommCtrl.h>
#include "resource.h"

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

            Gek::Xml::Document xmlDocument;
            if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
            {
                Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
                {
                    Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
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
            std::vector<Gek::Display::Mode> modeList = Gek::Display::getModes()[32];
            for(auto &mode : modeList)
            {
                CStringW aspectRatio(L"");
                switch (mode.aspectRatio)
                {
                case Gek::Display::AspectRatio::_4x3:
                    aspectRatio = L", (4x3)";
                    break;

                case Gek::Display::AspectRatio::_16x9:
                    aspectRatio = L", (16x9)";
                    break;

                case Gek::Display::AspectRatio::_16x10:
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
                std::vector<Gek::Display::Mode> modeList = Gek::Display::getModes()[32];
                UINT32 selectIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_GETCURSEL, 0, 0);
                auto &mode = modeList[selectIndex];

                Gek::Xml::Document xmlDocument;
                xmlDocument.load(L"%root%\\config.xml");
                Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                if (!xmlConfigNode || xmlConfigNode.getType().CompareNoCase(L"config") != 0)
                {
                    xmlDocument.create(L"config");
                    xmlConfigNode = xmlDocument.getRoot();
                }

                Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
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
    Gek::Engine::Core::Interface *engineCore = (Gek::Engine::Core::Interface *)GetWindowLongPtr(window, GWLP_USERDATA);
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

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR strCommandLine, _In_ int nCmdShow)
{
    static const LPCWSTR materialFormat = \
        L"<?xml version=\"1.0\"?>\r\n" \
        L"<material>\r\n" \
        L"    <shader>standard</shader>\r\n" \
        L"    <maps>\r\n" \
        L"        <albedo>%%directory%%\\rock.albedo</albedo>\r\n" \
        L"        <normal>%%directory%%\\rock.normal</normal>\r\n" \
        L"        <roughness>*color:%f</roughness>\r\n" \
        L"        <metalness>*color:%f</metalness>\r\n" \
        L"    </maps>\r\n" \
        L"</material>";

    static const LPCWSTR entityFormat = \
L"    <entity>\r\n" \
L"      <transform position=\"0,%d,%d\" />\r\n" \
/*L"      <color>lerp(.5,1,arand(1)),lerp(.5,1,arand(1)),lerp(.5,1,arand(1)),1</color>\r\n" \*/
L"      <model>*sphere|debug_%d_%d|1</model>\r\n" \
L"    </entity>\r\n";

    CStringW entities;
    for (UINT32 roughness = 0; roughness < 11; roughness++)
    {
        for (UINT32 metalness = 0; metalness < 11; metalness++)
        {
            CStringW material(Gek::String::format(materialFormat, (float(roughness) / 10.0f), (float(metalness) / 10.0f)));
            CStringW fileName(Gek::String::format(L"debug_%d_%d.xml", roughness, metalness));
            Gek::FileSystem::save((L"%root%\\data\\materials\\" + fileName), material);

            entities += Gek::String::format(entityFormat, ((roughness - 5) * 2) + 12, ((metalness - 5) * 2) - 2, roughness, metalness);
        }
    }

    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, DialogProc) == IDOK)
    {
        CComPtr<Gek::Context::Interface> context;
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
            CComPtr<Gek::Engine::Core::Interface> engineCore;
            context->createInstance(CLSID_IID_PPV_ARGS(Gek::Engine::Core::Class, &engineCore));
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
                kClass.lpszClassName = L"GEKvX_Engine_314159";
                if (RegisterClass(&kClass))
                {
                    UINT32 width = 800;
                    UINT32 height = 600;
                    bool windowed = true;

                    Gek::Xml::Document xmlDocument;
                    if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
                    {
                        Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                        if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
                        {
                            Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
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
                    HWND window = CreateWindow(L"GEKvX_Engine_314159", L"GEKvX Engine", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, centerPositionX, centerPositionY, windowWidth, windowHeight, 0, nullptr, GetModuleHandle(nullptr), 0);
                    if (window)
                    {
                        if (SUCCEEDED(engineCore->initialize(window)))
                        {
                            SetWindowLongPtr(window, GWLP_USERDATA, LONG((Gek::Engine::Core::Interface *)engineCore));
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