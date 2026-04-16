#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "API/Engine/Core.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <cstdio>

static void initializeConsoleOutput(void)
{
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        return;
    }

    FILE *stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
}
#endif

using namespace Gek;

#ifdef _WIN32
int CALLBACK wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance, _In_ wchar_t *commandLine, _In_ int commandShow)
#else
int main(int argumentCount, char const * const argumentList[])
#endif
{
#ifdef _WIN32
    initializeConsoleOutput();
#endif

    auto binaryPath(FileSystem::GetModuleFilePath().getParentPath());
    auto corePluginPath(binaryPath / "plugins" / "core");
    auto renderPluginPath(binaryPath / "plugins" / "render");
    auto systemPluginPath(binaryPath / "plugins" / "system");
    auto cachePath(FileSystem::GetCacheFromModule());
    auto rootPath(cachePath.getParentPath());

    cachePath.setWorkingDirectory();

    std::vector<FileSystem::Path> searchPathList;
    searchPathList.push_back(corePluginPath);

    auto configPath = binaryPath / "cache" / "config.json";
    auto config = Gek::JSON::Load(configPath);
    std::string renderer = config["renderer"] = Gek::JSON::Value(config, "renderer", std::string("renderd3d11"));
    Gek::JSON::Save(config, configPath);

    std::vector<FileSystem::Path> pluginList;
    pluginList.push_back(renderPluginPath / renderer);
    pluginList.push_back(systemPluginPath / "systemwin32");

    ContextPtr context(Context::Create(&searchPathList, &pluginList));
    if (context)
    {
        context->setCachePath(cachePath);

        auto gekDataPath = std::getenv("gek_data_path");
        if (gekDataPath)
        {
            context->addDataPath(gekDataPath);
        }

        context->addDataPath(rootPath / "data");
        context->addDataPath(rootPath.getString());

        Plugin::CorePtr core = context->createClass<Plugin::Core>("Engine::Core");
    }

    return 0;
}