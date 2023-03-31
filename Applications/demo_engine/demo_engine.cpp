#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"

using namespace Gek;

int CALLBACK wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance, _In_ wchar_t *commandLine, _In_ int commandShow)
{
    auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
    auto rootPath(pluginPath.getParentPath());
	auto cachePath(rootPath / "cache"sv);
    SetCurrentDirectoryW(cachePath.getWindowsString().data());

    std::vector<FileSystem::Path> searchPathList;
    searchPathList.push_back(pluginPath);

    ContextPtr context(Context::Create(searchPathList));
    if (context)
    {
		context->setCachePath(cachePath);

        wchar_t gekDataPath[MAX_PATH + 1] = L"\0";
        if (GetEnvironmentVariable(L"gek_data_path", gekDataPath, MAX_PATH) > 0)
        {
            context->addDataPath(String::Narrow(gekDataPath));
        }

        context->addDataPath(rootPath / "data"sv);
        context->addDataPath(rootPath.getString());

        Engine::CorePtr core(context->createClass<Engine::Core>("Engine::Core", (Window *)nullptr));
        if (core)
        {
            while (core->update())
            {
            };
        }
    }

    return 0;
}