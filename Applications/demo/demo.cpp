#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include <experimental\filesystem>

using namespace Gek;

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ wchar_t *strCommandLine, _In_ int nCmdShow)
{
    try
    {
        String currentModuleName((MAX_PATH + 1), L' ');
        GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
        currentModuleName.trimRight();

        String fullModuleName((MAX_PATH + 1), L' ');
        GetFullPathName(currentModuleName, MAX_PATH, &fullModuleName.at(0), nullptr);
        fullModuleName.trimRight();

        std::experimental::filesystem::path fullModulePath(fullModuleName);
        fullModulePath.remove_filename();
        String pluginPath(fullModulePath.wstring());

        SetCurrentDirectory(pluginPath);
        std::vector<String> searchPathList;
        searchPathList.push_back(pluginPath);

        fullModulePath.remove_filename();
        String rootPath(fullModulePath.wstring());

        ContextPtr context(Context::Create(rootPath, searchPathList));
        if (true)
        {
            Plugin::CorePtr core(context->createClass<Plugin::Core>(L"Engine::Core", (Window *)nullptr));
            while (core->update())
            {
            };
        }
    }
    catch (const std::exception &exception)
    {
        MessageBoxA(nullptr, StringUTF8::Format("Caught: %v\r\nType: %v", exception.what(), typeid(exception).name()), "GEK Engine - Error", MB_OK | MB_ICONERROR);
    }
    catch (...)
    {
        MessageBox(nullptr, L"Caught: Non-standard exception", L"GEK Engine - Error", MB_OK | MB_ICONERROR);
    };

    return 0;
}