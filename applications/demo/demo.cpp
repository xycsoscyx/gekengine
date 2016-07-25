#include "GEK\Utility\Trace.h"
#include "GEK\Utility\Display.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Application.h"
#include "GEK\Engine\Core.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "resource.h"

using namespace Gek;

auto displayModes = getDisplayModes();

INT_PTR CALLBACK DialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(dialog, IDCANCEL);
        return TRUE;

    case WM_INITDIALOG:
    {
        XmlDocumentPtr document;
        XmlNodePtr configurationNode;
        try
        {
            document = XmlDocument::load(L"$root\\config.xml");
            configurationNode = document->getRoot(L"config");
        }
        catch (const Exception &)
        {
            document = XmlDocument::create(L"config");
            configurationNode = document->getRoot(L"config");
        };

        XmlNodePtr displayNode(configurationNode->firstChildElement(L"display"));
        uint32_t width = displayNode->getAttribute(L"width", L"800");
        uint32_t height = displayNode->getAttribute(L"height", L"600");
        bool fullscreen = displayNode->getAttribute(L"fullscreen", L"false");

        uint32_t selectIndex = 0;
        SendDlgItemMessage(dialog, IDC_MODES, CB_RESETCONTENT, 0, 0);

        auto modesRange = displayModes.equal_range(32);
        for (auto &modeSearch = modesRange.first; modeSearch != modesRange.second; ++modeSearch)
        {
            auto &mode = (*modeSearch).second;

            String aspectRatio(L"");
            switch (mode.aspectRatio)
            {
            case AspectRatio::_4x3:
                aspectRatio = L", (4x3)";
                break;

            case AspectRatio::_16x9:
                aspectRatio = L", (16x9)";
                break;

            case AspectRatio::_16x10:
                aspectRatio = L", (16x10)";
                break;
            };

            String modeString(String(L"%vx%v%v", mode.width, mode.height, aspectRatio));
            int modeIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_ADDSTRING, 0, (WPARAM)modeString.c_str());
            SendDlgItemMessage(dialog, IDC_MODES, CB_SETITEMDATA, modeIndex, (WPARAM)&mode);
            if (mode.width == width && mode.height == height)
            {
                selectIndex = modeIndex;
            }
        }

        SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_SETCHECK, fullscreen ? BST_CHECKED : BST_UNCHECKED, 0);

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
            uint32_t selectIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_GETCURSEL, 0, 0);
            auto &mode = *(DisplayMode *)SendDlgItemMessage(dialog, IDC_MODES, CB_GETITEMDATA, selectIndex, 0);

            XmlDocumentPtr document;
            XmlNodePtr configurationNode;
            try
            {
                document = XmlDocument::load(L"$root\\config.xml");
                configurationNode = document->getRoot(L"config");
            }
            catch (const Exception &)
            {
                document = XmlDocument::create(L"config");
                configurationNode = document->getRoot(L"config");
            };

            XmlNodePtr displayNode(configurationNode->firstChildElement(L"display", true));
            displayNode->setAttribute(L"width", String(L"%v", mode.width));
            displayNode->setAttribute(L"height", String(L"%v", mode.height));
            displayNode->setAttribute(L"fullscreen", SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_GETCHECK, 0, 0) == BST_CHECKED ? L"true" : L"false");
            document->save(L"$root\\config.xml");

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

LRESULT CALLBACK WindowProc(HWND window, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    LRESULT resultValue = 0;
    Application *application = reinterpret_cast<Application *>(GetWindowLongPtr(window, GWLP_USERDATA));
    switch (message)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        if (application)
        {
            application->windowEvent(message, wParam, lParam);
        }

        PostQuitMessage(0);
        break;

    default:
        if (application)
        {
            resultValue = application->windowEvent(message, wParam, lParam);
        }

        break;
    };

    if (resultValue == 0)
    {
        resultValue = DefWindowProc(window, message, wParam, lParam);
    }

    return resultValue;
}

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ wchar_t *strCommandLine, _In_ int nCmdShow)
{
    try
    {
        Trace::initialize();
        if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, DialogProc) == IDOK)
        {
            std::vector<String> searchPathList;

#ifdef _DEBUG
            SetCurrentDirectory(FileSystem::expandPath(L"$root\\Debug"));
            searchPathList.push_back(L"$root\\Debug\\Plugins");
#else
            SetCurrentDirectory(FileSystem::expandPath(L"$root\\Release"));
            searchPathList.push_back(L"$root\\Release\\Plugins");
#endif

            ContextPtr context(Context::create(searchPathList));

            WNDCLASS windowClass;
            windowClass.style = 0;
            windowClass.lpfnWndProc = WindowProc;
            windowClass.cbClsExtra = 0;
            windowClass.cbWndExtra = 0;
            windowClass.hInstance = GetModuleHandle(nullptr);
            windowClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
            windowClass.hCursor = nullptr;
            windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            windowClass.lpszMenuName = nullptr;
            windowClass.lpszClassName = L"GEKvX_Engine_Demo";
            ATOM classAtom = RegisterClass(&windowClass);
            GEK_CHECK_CONDITION(!classAtom, Trace::Exception, "Unable to register window class: %v", GetLastError());

            XmlDocumentPtr document;
            XmlNodePtr configurationNode;
            try
            {
                document = XmlDocument::load(L"$root\\config.xml");
                configurationNode = document->getRoot(L"config");
            }
            catch (const Exception &)
            {
                document = XmlDocument::create(L"config");
                configurationNode = document->getRoot(L"config");
            };

            XmlNodePtr displayNode(configurationNode->firstChildElement(L"display"));
            uint32_t width = displayNode->getAttribute(L"width", L"800");
            uint32_t height = displayNode->getAttribute(L"height", L"600");
            bool fullscreen = displayNode->getAttribute(L"fullscreen", L"false");

            RECT clientRect;
            clientRect.left = 0;
            clientRect.top = 0;
            clientRect.right = width;
            clientRect.bottom = height;
            AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, false);
            int windowWidth = (clientRect.right - clientRect.left);
            int windowHeight = (clientRect.bottom - clientRect.top);
            int centerPositionX = (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - ((clientRect.right - clientRect.left) / 2);
            int centerPositionY = (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - ((clientRect.bottom - clientRect.top) / 2);
            HWND window = CreateWindow(L"GEKvX_Engine_Demo", L"GEKvX Application - Demo", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, centerPositionX, centerPositionY, windowWidth, windowHeight, 0, nullptr, GetModuleHandle(nullptr), 0);
            GEK_CHECK_CONDITION(window == nullptr, Trace::Exception, "Unable to create window: %v", GetLastError());

            ApplicationPtr application(context->createClass<Application>(L"Engine::Core", window));

            SetWindowLongPtr(window, GWLP_USERDATA, LONG(application.get()));
            ShowWindow(window, SW_SHOW);
            UpdateWindow(window);

            MSG message = { 0 };
            while (message.message != WM_QUIT)
            {
                while (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                };

                if (!application->update())
                {
                    break;
                }
            };

            DestroyWindow(window);
            SetWindowLongPtr(window, GWLP_USERDATA, 0);
        }
    }
    catch (const Exception &exception)
    {
        std::string message;
        message = exception.what();
        message += "\r\n";
        message += exception.in();
        message += ": ";
        message += exception.at();

        MessageBoxA(nullptr, message.c_str(), "GEK Runtime Error", MB_OK | MB_ICONERROR);
    }
    catch (...)
    {
        MessageBox(nullptr, L"Unhandled Exception", L"GEK Runtime Error", MB_OK | MB_ICONERROR);
    };

    try
    {
        Trace::shutdown();
    }
    catch (...)
    {
    };

    return 0;
}