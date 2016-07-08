#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\ContextUser.h"
#include <Windows.h>
#include <unordered_map>
#include <list>

namespace Gek
{
    class ContextImplementation
        : public Context
    {
    private:
        std::list<HMODULE> moduleList;
        std::unordered_map<String, std::function<ContextUserPtr(Context *, void *)>> classMap;
        std::unordered_multimap<String, String> typeMap;

    public:
        ContextImplementation(std::vector<String> searchPathList)
        {
            GEK_TRACE_FUNCTION();

            searchPathList.push_back(L"$root");
            for (auto &searchPath : searchPathList)
            {
                FileSystem::find(searchPath, L"*.dll", false, [&](const wchar_t *fileName) -> bool
                {
                    HMODULE module = LoadLibrary(fileName);
                    if (module)
                    {
                        InitializePlugin initializePlugin = (InitializePlugin)GetProcAddress(module, "initializePlugin");
                        if (initializePlugin)
                        {
                            GEK_TRACE_EVENT("Plugin found", GEK_PARAMETER(fileName));

                            initializePlugin([this](const wchar_t *className, std::function<ContextUserPtr(Context *, void *)> creator) -> void
                            {
                                if (classMap.count(className) == 0)
                                {
                                    classMap[className] = creator;
                                }
                                else
                                {
                                    GEK_TRACE_ERROR("Duplicate class entry located", GEK_PARAMETER(className));
                                }
                            }, [this](const wchar_t *typeName, const wchar_t *className) -> void
                            {
                                typeMap.insert(std::make_pair(typeName, className));
                            });

                            moduleList.push_back(module);
                        }
                        else
                        {
                            FreeLibrary(module);
                        }
                    }
                    else
                    {
                        DWORD errorCode = GetLastError();
                        GEK_TRACE_ERROR("Unable to load plugin", GEK_PARAMETER(fileName), GEK_PARAMETER(errorCode));
                    }

                    return true;
                });
            }
        }

        ~ContextImplementation(void)
        {
            typeMap.clear();
            classMap.clear();
            for (auto &module : moduleList)
            {
                FreeLibrary(module);
            }
        }

        // Context
        ContextUserPtr createBaseClass(const wchar_t *name, void *arguments) const
        {
            auto classSearch = classMap.find(name);
            GEK_CHECK_CONDITION(classSearch == classMap.end(), Trace::Exception, "Unable to find requested class creator: %v", name);

            return (*classSearch).second((Context *)this, arguments);
        }

        void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) const
        {
            GEK_REQUIRE(typeName);
            GEK_REQUIRE(onType);

            auto typeRange = typeMap.equal_range(typeName);
            for (auto typeSearch = typeRange.first; typeSearch != typeRange.second; ++typeSearch)
            {
                onType((*typeSearch).second);
            }
        }
    };

    ContextPtr Context::create(const std::vector<String> &searchPathList)
    {
        GEK_TRACE_FUNCTION();
        return makeShared<Context, ContextImplementation>(searchPathList);
    }
};
