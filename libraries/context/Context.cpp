#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\COM.h"
#include "GEK\Context\UnknownMixin.h"
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
    class ContextImplementation : virtual public UnknownMixin
        , virtual public ObservableMixin
        , virtual public Context
    {
    private:
        std::list<CStringW> searchPathList;

        std::list<HMODULE> moduleList;
        std::unordered_map<CLSID, std::function<HRESULT(ContextUser **)>> classList;
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

        // IUnknown
        BEGIN_INTERFACE_LIST(ContextImplementation)
            INTERFACE_LIST_ENTRY_COM(Context)
            INTERFACE_LIST_ENTRY_COM(Observable)
        END_INTERFACE_LIST_UNKNOWN

        Context *getContext(void)
        {
            return this;
        }

        // Context
        STDMETHODIMP_(void) addSearchPath(LPCWSTR fileName)
        {
            searchPathList.push_back(fileName);
        }

        STDMETHODIMP_(void) initialize(void)
        {
            searchPathList.push_back(L"%root%");
            for (auto &searchPath : searchPathList)
            {
                Gek::FileSystem::find(searchPath,L"*.dll", false, [&](LPCWSTR fileName) -> HRESULT
                {
                    HMODULE module = LoadLibrary(fileName);
                    if (module)
                    {
                        typedef HRESULT(*GEKGETMODULECLASSES)(std::unordered_map<CLSID, std::function<HRESULT(ContextUser **)>> &, std::unordered_map<CLSID, std::vector<CLSID >> &);
                        GEKGETMODULECLASSES getModuleClasses = (GEKGETMODULECLASSES)GetProcAddress(module, "GEKGetModuleClasses");
                        if (getModuleClasses)
                        {
                            moduleList.push_back(module);
                            std::unordered_map<CLSID, std::function<HRESULT(ContextUser **)>> moduleClassList;
                            std::unordered_map<CLSID, std::vector<CLSID>> moduleTypedClassList;

                            if (SUCCEEDED(getModuleClasses(moduleClassList, moduleTypedClassList)))
                            {
                                for (auto &moduleClass : moduleClassList)
                                {
                                    if (classList.find(moduleClass.first) == classList.end())
                                    {
                                        classList[moduleClass.first] = moduleClass.second;
                                    }
                                    else
                                    {
                                    }
                                }

                                for (auto &moduleTypedClass : moduleTypedClassList)
                                {
                                    typedClassList[moduleTypedClass.first].insert(typedClassList[moduleTypedClass.first].end(), moduleTypedClass.second.begin(), moduleTypedClass.second.end());
                                }
                            }
                            else
                            {
                            }
                        }
                    }
                    else
                    {
                        DWORD errorCode = GetLastError();
                    }

                    return S_OK;
                });
            }
        }

        STDMETHODIMP createInstance(REFGUID className, REFIID interfaceType, LPVOID FAR *returnObject)
        {
            GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            auto classIterator = classList.find(className);
            if (classIterator != classList.end())
            {
                CComPtr<ContextUser> classInstance;
                resultValue = ((*classIterator).second)(&classInstance);
                if (SUCCEEDED(resultValue) && classInstance)
                {
                    classInstance->registerContext(this);
                    resultValue = classInstance->QueryInterface(interfaceType, returnObject);
                }
            }

            return resultValue;
        }

        STDMETHODIMP createEachType(REFCLSID typeName, std::function<HRESULT(REFCLSID, IUnknown *)> onCreateInstance)
        {
            HRESULT resultValue = S_OK;
            auto typedClassIterator = typedClassList.find(typeName);
            if (typedClassIterator != typedClassList.end())
            {
                for (auto &className : (*typedClassIterator).second)
                {
                    CComPtr<IUnknown> classInstance;
                    resultValue = createInstance(className, IID_PPV_ARGS(&classInstance));
                    if (classInstance)
                    {
                        resultValue = onCreateInstance(className, classInstance);
                        if (FAILED(resultValue))
                        {
                            break;
                        }
                    }
                };
            }

            return resultValue;
        }
    };

    HRESULT Context::create(Context **returnObject)
    {
        GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);

        HRESULT resultValue = E_OUTOFMEMORY;
        CComPtr<ContextImplementation> context(new ContextImplementation());
        _ASSERTE(context);
        if (context)
        {
            resultValue = context->QueryInterface(IID_PPV_ARGS(returnObject));
        }

        return resultValue;
    }
};
