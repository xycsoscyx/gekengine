#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Utility\Context.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Application.hpp"
#include "GEK\Engine\Core.hpp"
#include <experimental\filesystem>
#include <Windows.h>
#include <CommCtrl.h>
#include "resource.h"

using namespace Gek;

LRESULT CALLBACK WindowProc(HWND window, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
        
    Application::Event eventData(message, wParam, lParam);
    Application *application = reinterpret_cast<Application *>(GetWindowLongPtr(window, GWLP_USERDATA));
    if (application)
    {
        auto resultValue = application->windowEvent(eventData);
        if (resultValue.handled)
        {
            return resultValue.result;
        }
    }

    return DefWindowProc(window, message, wParam, lParam);
}

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ wchar_t *strCommandLine, _In_ int nCmdShow)
{
    try
    {
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

        WNDCLASS windowClass;
        windowClass.style = 0;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.cbClsExtra = 0;
        windowClass.cbWndExtra = 0;
        windowClass.hInstance = GetModuleHandle(nullptr);
        windowClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
        windowClass.hCursor = LoadCursor(GetModuleHandle(nullptr), IDC_ARROW);
        windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        windowClass.lpszMenuName = nullptr;
        windowClass.lpszClassName = L"GEKvX_Engine_Demo";
        ATOM classAtom = RegisterClass(&windowClass);
        if (!classAtom)
        {
            throw std::exception("Unable to register window class");
        }

        RECT clientRectangle;
        clientRectangle.left = 0;
        clientRectangle.top = 0;
        clientRectangle.right = 1;
        clientRectangle.bottom = 1;
        AdjustWindowRect(&clientRectangle, WS_OVERLAPPEDWINDOW, false);
        int windowWidth = (clientRectangle.right - clientRectangle.left);
        int windowHeight = (clientRectangle.bottom - clientRectangle.top);
        HWND window = CreateWindow(windowClass.lpszClassName, L"GEKvX Application - Demo", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        if (window == nullptr)
        {
            throw std::exception("Unable to create window");
        }

        ContextPtr context(Context::create(rootPath, searchPathList));
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