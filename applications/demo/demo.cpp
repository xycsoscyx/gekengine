﻿#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\Display.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Application.h"
#include "GEK\Engine\Core.h"
#include <experimental\filesystem>
#include <Windows.h>
#include <CommCtrl.h>
#include "resource.h"

using namespace Gek;

Display::ModesList modesList;

INT_PTR CALLBACK DialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    Context *context = (Context *)GetWindowLongPtr(dialog, GWLP_USERDATA);
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(dialog, IDCANCEL);
        return TRUE;

    case WM_INITDIALOG:
    {
        context = (Context *)lParam;
        SetWindowLongPtr(dialog, GWLP_USERDATA, (LONG)context);
        Xml::Node configRoot(nullptr);
        try
        {
            configRoot = Xml::load(context->getFileName(L"config.xml"), L"config");
        }
        catch (const Xml::Exception &)
        {
            configRoot = Xml::Node(L"config");
        };

        auto &displayNode = configRoot.getChild(L"display");
        uint32_t width = displayNode.getAttribute(L"width", L"800");
        uint32_t height = displayNode.getAttribute(L"height", L"600");
        bool fullscreen = displayNode.getAttribute(L"fullscreen", L"false");

        uint32_t selectIndex = 0;
        SendDlgItemMessage(dialog, IDC_MODES, CB_RESETCONTENT, 0, 0);
		for(auto &mode : modesList)
		{
            String aspectRatio(L"");
            switch (mode.aspectRatio)
            {
			case Display::AspectRatio::_4x3:
                aspectRatio = L" (4x3)";
                break;

            case Display::AspectRatio::_16x9:
                aspectRatio = L" (16x9)";
                break;

            case Display::AspectRatio::_16x10:
                aspectRatio = L" (16x10)";
                break;
            };

            int modeIndex = SendDlgItemMessage(dialog, IDC_MODES, CB_ADDSTRING, 0, (WPARAM)String::create(L"%vx%v%v", mode.width, mode.height, aspectRatio).c_str());
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
            auto &mode = *(Display::Mode *)SendDlgItemMessage(dialog, IDC_MODES, CB_GETITEMDATA, selectIndex, 0);

            Xml::Node configRoot(nullptr);
            try
            {
				configRoot = Xml::load(context->getFileName(L"config.xml"), L"config");
			}
            catch (const Xml::Exception &)
            {
                configRoot = Xml::Node(L"config");
            };

            auto &displayNode = configRoot.getChild(L"display");
            displayNode.attributes[L"width"] = mode.width;
            displayNode.attributes[L"height"] = mode.height;
            displayNode.attributes[L"fullscreen"] = (SendDlgItemMessage(dialog, IDC_FULLSCREEN, BM_GETCHECK, 0, 0) == BST_CHECKED ? L"true" : L"false");
			Xml::save(configRoot, context->getFileName(L"config.xml"));

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
		modesList = Display().getModes(32);

        String rootPath;
        String currentModuleName((MAX_PATH + 1), L' ');
        GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
        currentModuleName.trimRight();

        String fullModuleName((MAX_PATH + 1), L' ');
        GetFullPathName(currentModuleName, MAX_PATH, &fullModuleName.at(0), nullptr);
        fullModuleName.trimRight();

        std::experimental::filesystem::path fullModulePath(fullModuleName);
        fullModulePath.remove_filename();
        fullModulePath.remove_filename();

        rootPath = fullModulePath.wstring();

        std::vector<String> searchPathList;

#ifdef _DEBUG
        SetCurrentDirectory(FileSystem::getFileName(rootPath, L"Debug"));
        searchPathList.push_back(FileSystem::getFileName(rootPath, L"Debug\\Plugins"));
#else
        SetCurrentDirectory(FileSystem::getFileName(rootPath, L"Release"));
        searchPathList.push_back(FileSystem::getFileName(rootPath, L"Release\\Plugins"));
#endif

        ContextPtr context(Context::create(rootPath, searchPathList));
        if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, DialogProc, (LPARAM)context.get()) == IDOK)
        {
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
            if (!classAtom)
            {
                throw std::exception("Unable to register window class");
            }

            Xml::Node configRoot(nullptr);
            try
            {
				configRoot = Xml::load(context->getFileName(L"config.xml"), L"config");
			}
            catch (const Xml::Exception &)
            {
                configRoot = Xml::Node(L"config");
            };

            auto &displayNode = configRoot.getChild(L"display");
            uint32_t width = displayNode.getAttribute(L"width", L"800");
            uint32_t height = displayNode.getAttribute(L"height", L"600");
            bool fullscreen = displayNode.getAttribute(L"fullscreen", L"false");

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
            if (window == nullptr)
            {
                throw std::exception("Unable to create window");
            }

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
    catch (const std::exception &exception)
    {
        MessageBoxA(nullptr, exception.what(), "Unhandled Exception Error", MB_OK | MB_ICONERROR);
    }
    catch (...)
    {
        MessageBox(nullptr, L"Unhandled Exception", L"Runtime Error", MB_OK | MB_ICONERROR);
    };

    return 0;
}