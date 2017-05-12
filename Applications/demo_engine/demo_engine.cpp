#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"

using namespace Gek;

void test(std::string const &test)
{
	std::cout << test;
}

template <typename... PARAMETERS>
void vtest(const PARAMETERS&... values)
{
	test(values...);
}

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ wchar_t *strCommandLine, _In_ int nCmdShow)
{
	vtest("test");
	vtest("test"s);
	vtest(u8"test");
	vtest(u8"test"s);

	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	try
    {
        auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
        auto rootPath(pluginPath.getParentPath());

        std::vector<FileSystem::Path> searchPathList;
        searchPathList.push_back(pluginPath);

        ContextPtr context(Context::Create(rootPath, searchPathList));
        if (true)
        {
            Plugin::CorePtr core(context->createClass<Plugin::Core>("Engine::Core"s, (Window *)nullptr));
            while (core->update())
            {
            };
        }
    }
    catch (const std::exception &exception)
    {
        MessageBoxA(nullptr, String::Format("Caught: %v\r\nType: %v", exception.what(), typeid(exception).name()).c_str(), "GEK Engine - Error", MB_OK | MB_ICONERROR);
    }
    catch (...)
    {
        MessageBoxA(nullptr, "Caught: Non-standard exception", "GEK Engine - Error", MB_OK | MB_ICONERROR);
    };

    return 0;
}