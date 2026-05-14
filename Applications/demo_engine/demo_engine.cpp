#include "API/Engine/Core.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"

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
int main(int argumentCount, char const *const argumentList[])
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
    auto renderOptions = Gek::JSON::Find(config, "render");
#ifdef _WIN32
    const std::string defaultRenderDevice = "renderd3d11";
    const std::string systemPluginName = "systemwin32";
#else
    const std::string defaultRenderDevice = "rendervulkan";
    const std::string systemPluginName = "systemwayland";
#endif
    std::string renderDevice = Gek::JSON::Value(renderOptions, "device", defaultRenderDevice);
    renderOptions["device"] = renderDevice;
    config["render"] = renderOptions;
    Gek::JSON::Save(config, configPath);

    std::vector<FileSystem::Path> pluginList;
    pluginList.push_back(renderPluginPath / renderDevice);
    pluginList.push_back(systemPluginPath / systemPluginName);

    ContextPtr context(Context::Create(&searchPathList, &pluginList));
    if (context)
    {
        context->setCachePath(cachePath);

        auto addDataPathIfDirectory = [&](const FileSystem::Path &path, const char *label) -> bool
        {
            if (!path.isDirectory())
            {
                return false;
            }

            context->addDataPath(path);
            context->log(Context::Info, "demo_engine data path [{}]: {}", label, path.getString());
            return true;
        };

        auto gekDataPath = std::getenv("gek_data_path");
        if (gekDataPath)
        {
            addDataPathIfDirectory(FileSystem::Path(gekDataPath), "env:gek_data_path");
        }
        else
        {
            auto gekDataPathUpper = std::getenv("GEK_DATA_PATH");
            if (gekDataPathUpper)
            {
                addDataPathIfDirectory(FileSystem::Path(gekDataPathUpper), "env:GEK_DATA_PATH");
            }
        }

        addDataPathIfDirectory(rootPath / "data", "cacheRoot/data");
        addDataPathIfDirectory(rootPath, "cacheRoot");

        bool discoveredRepositoryData = false;
        auto probePath = binaryPath;
        for (uint32_t probeDepth = 0; probeDepth < 8; ++probeDepth)
        {
            auto candidateDataPath = probePath / "data";
            if (candidateDataPath.isDirectory())
            {
                discoveredRepositoryData = addDataPathIfDirectory(candidateDataPath, "auto-discovered");
                break;
            }

            auto parentPath = probePath.getParentPath();
            if (parentPath.getString() == probePath.getString())
            {
                break;
            }

            probePath = parentPath;
        }

        if (!discoveredRepositoryData)
        {
            context->log(
                Context::Warning,
                "demo_engine could not auto-discover a data directory from binary path '{}' (set gek_data_path or GEK_DATA_PATH if assets are missing)",
                binaryPath.getString());
        }

        Plugin::CorePtr core = context->createClass<Plugin::Core>("Engine::Core");
    }

    return 0;
}