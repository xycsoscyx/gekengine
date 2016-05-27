#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\ContextUser.h"
#include <unordered_map>
#include <list>

namespace Gek
{
    class ContextImplementation
        : public Context
    {
    private:
        std::list<HMODULE> moduleList;
        std::unordered_map<std::wstring, std::function<ContextUserPtr(Context *, void *)>> classMap;
        std::unordered_multimap<std::wstring, std::wstring> typeMap;

    public:
        ContextImplementation(std::vector<wstring> searchPathList)
        {
            GEK_TRACE_FUNCTION();

            searchPathList.push_back(L"$root");
            for (auto &searchPath : searchPathList)
            {
                Gek::FileSystem::find(searchPath.c_str(), L"*.dll", false, [&](const wchar_t *fileName) -> bool
                {
                    HMODULE module = LoadLibrary(fileName);
                    if (module)
                    {
                        typedef void(*InitializePlugin)(std::function<void(const wchar_t *, std::function<ContextUserPtr(Context *, void *)>)> addClass, std::function<void(const wchar_t *, const wchar_t *)> addType);
                        InitializePlugin initializePlugin = (InitializePlugin)GetProcAddress(module, "GEKInitializePlugin");
                        if (initializePlugin)
                        {
                            GEK_TRACE_EVENT("Plugin found: %", GEK_PARAMETER(fileName));

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
        ContextUserPtr createBaseClass(const wchar_t *name, void *parameters) const
        {
            auto classIterator = classMap.find(name);
            GEK_THROW_ERROR(classIterator == classMap.end(), BaseException, "Unable to find requested class creator: %", name);
            return (*classIterator).second((Context *)this, parameters);
        }

        void listTypes(const wchar_t *typeName, std::function<void(const wchar_t *)> onType) const
        {
            GEK_REQUIRE(typeName);
            GEK_REQUIRE(onType);

            auto typeIterator = typeMap.equal_range(typeName);
            for (auto iterator = typeIterator.first; iterator != typeIterator.second; ++iterator)
            {
                onType((*iterator).second.c_str());
            }
        }
    };

    ContextPtr Context::create(const std::vector<wstring> &searchPathList)
    {
        GEK_TRACE_FUNCTION();
        return std::remake_shared<Context, ContextImplementation>(searchPathList);
    }
};
