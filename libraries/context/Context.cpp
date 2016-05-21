#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\COM.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Context\Context.h"
#include "GEK\Context\ContextUser.h"
#include <atlbase.h>
#include <atlstr.h>
#include <atlpath.h>
#include <list>
#include <unordered_map>

namespace Gek
{
    class ContextImplementation
        : virtual public ObservableMixin
        , virtual public Context
    {
    private:
        std::list<CStringW> searchPathList;

        std::list<HMODULE> moduleList;
        std::unordered_map<CLSID, std::function<std::shared_ptr<Gek::ContextUser>(void)>> classList;
        std::unordered_map<CLSID, std::vector<CLSID>> typedClassList;

    public:
        ContextImplementation(void)
        {
        }

        ~ContextImplementation(void)
        {
            classList.clear();
            typedClassList.clear();
            for (auto &module : moduleList)
            {
                FreeLibrary(module);
            }
        }

        Context *getContext(void)
        {
            return this;
        }

        // Context
        void addSearchPath(LPCWSTR fileName)
        {
            GEK_TRACE_EVENT("Adding plugin search path: %S", GEK_PARAMETER(fileName));
            searchPathList.push_back(fileName);
        }

        void initialize(void)
        {
            GEK_TRACE_FUNCTION();

            searchPathList.push_back(L"%root%");
            for (auto &searchPath : searchPathList)
            {
                Gek::FileSystem::find(searchPath,L"*.dll", false, [&](LPCWSTR fileName) -> bool
                {
                    HMODULE module = LoadLibrary(fileName);
                    if (module)
                    {
                        typedef void(*GEKGETMODULECLASSES)(std::unordered_map<CLSID, std::function<std::shared_ptr<Gek::ContextUser>(void)>> &, std::unordered_map<CLSID, std::vector<CLSID >> &);
                        GEKGETMODULECLASSES getModuleClasses = (GEKGETMODULECLASSES)GetProcAddress(module, "GEKGetModuleClasses");
                        if (getModuleClasses)
                        {
                            GEK_TRACE_EVENT("Plugin found: %S", GEK_PARAMETER(fileName));

                            moduleList.push_back(module);
                            std::unordered_map<CLSID, std::function<std::shared_ptr<Gek::ContextUser>(void)>> moduleClassList;
                            std::unordered_map<CLSID, std::vector<CLSID>> moduleTypedClassList;
                            getModuleClasses(moduleClassList, moduleTypedClassList);
                            for (auto &moduleClass : moduleClassList)
                            {
                                if (classList.find(moduleClass.first) == classList.end())
                                {
                                    classList[moduleClass.first] = moduleClass.second;
                                }
                                else
                                {
                                    GEK_TRACE_ERROR("Duplicate class entry located", GEK_PARAMETER(moduleClass.first));
                                }
                            }

                            for (auto &moduleTypedClass : moduleTypedClassList)
                            {
                                typedClassList[moduleTypedClass.first].insert(typedClassList[moduleTypedClass.first].end(), moduleTypedClass.second.begin(), moduleTypedClass.second.end());
                            }
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

        std::shared_ptr<Gek::ContextUser> createInstance(REFGUID className)
        {
            auto classIterator = classList.find(className);
            GEK_CHECK_EXCEPTION(classIterator == classList.end(), BaseException, "Unable to find requested class");

            std::shared_ptr<Gek::ContextUser> object((*classIterator).second());
            object->registerContext(this);
            return object;
        }

        void createEachType(REFCLSID typeName, std::function<void(REFCLSID, std::shared_ptr<Gek::ContextUser>)> onCreateInstance)
        {
            GEK_REQUIRE(onCreateInstance);

            auto typedClassIterator = typedClassList.find(typeName);
            if (typedClassIterator != typedClassList.end())
            {
                for (auto &className : (*typedClassIterator).second)
                {
                    std::shared_ptr<Gek::ContextUser> object(createInstance(className));
                    onCreateInstance(className, object);
                };
            }
        }
    };

    std::shared_ptr<Context> Context::create(void)
    {
        GEK_TRACE_FUNCTION();
        return std::dynamic_pointer_cast<Context>(std::make_shared<ContextImplementation>());
    }
};
