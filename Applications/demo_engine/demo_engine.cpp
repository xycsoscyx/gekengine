#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"

using namespace Gek;

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ wchar_t *strCommandLine, _In_ int nCmdShow)
{
    try
    {
        auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
        auto rootPath(pluginPath.getParentPath());

        std::vector<FileSystem::Path> searchPathList;
        searchPathList.push_back(pluginPath);

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
        MessageBoxA(nullptr, CString::Format("Caught: %v\r\nType: %v", exception.what(), typeid(exception).name()), "GEK Engine - Error", MB_OK | MB_ICONERROR);
    }
    catch (...)
    {
        MessageBox(nullptr, L"Caught: Non-standard exception", L"GEK Engine - Error", MB_OK | MB_ICONERROR);
    };

    return 0;
}