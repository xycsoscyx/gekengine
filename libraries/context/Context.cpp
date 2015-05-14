#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "Context\Common.h"
#include "Context\ContextInterface.h"
#include "Context\ContextUserInterface.h"
#include "Context\Observable.h"
#include <atlbase.h>
#include <atlstr.h>
#include <list>
#include <unordered_map>

namespace Gek
{
    class Context : public Observable
                  , public ContextInterface
    {
    private:
        ULONG referenceCount;

        std::list<CStringW> searchPaths;

        std::list<HMODULE> modules;
        std::unordered_map<CLSID, std::function<HRESULT(ContextUserInterface **)>> classes;
        std::unordered_map<CLSID, std::vector<CLSID>> typedClasses;

    public:
        ~Context(void)
        {
            classes.clear();
            typedClasses.clear();
            for (auto &module : modules)
            {
                FreeLibrary(module);
            }
        }

        // IUnknown
        STDMETHODIMP_(ULONG) AddRef(void)
        {
            return InterlockedIncrement(&referenceCount);
        }

        STDMETHODIMP_(ULONG) Release(void)
        {
            LONG newReferenceCount = InterlockedDecrement(&referenceCount);
            if (newReferenceCount == 0)
            {
                delete this;
            }

            return newReferenceCount;
        }

        STDMETHODIMP QueryInterface(REFIID interfaceID, LPVOID FAR *object)
        {
            REQUIRE_RETURN(object, E_INVALIDARG);

            HRESULT returnValue = E_INVALIDARG;
            if (IsEqualIID(IID_IUnknown, interfaceID))
            {
                AddRef();
                (*object) = dynamic_cast<IUnknown *>(dynamic_cast<ContextInterface *>(this));
                _ASSERTE(*object);
                returnValue = S_OK;
            }
            else if (IsEqualIID(__uuidof(ContextInterface), interfaceID))
            {
                AddRef();
                (*object) = dynamic_cast<ContextInterface *>(this);
                _ASSERTE(*object);
                returnValue = S_OK;
            }

            return returnValue;
        }

        // ContextInterface
        STDMETHODIMP_(void) addSearchPath(LPCWSTR basePath)
        {
            searchPaths.push_back(basePath);
        }

        STDMETHODIMP_(void) initialize(void)
        {
            searchPaths.push_back(L"%root%");
            for (auto &searchPath : searchPaths)
            {
                Gek::FileSystem::find(searchPath, L"*.dll", false, [&](LPCWSTR fileName) -> HRESULT
                {
                    HMODULE module = LoadLibrary(fileName);
                    if (module)
                    {
                        typedef HRESULT(*GEKGETMODULECLASSES)(std::unordered_map<CLSID, std::function<HRESULT(ContextUserInterface **)>> &, std::unordered_map<CLSID, std::vector<CLSID >> &);
                        GEKGETMODULECLASSES getModuleClasses = (GEKGETMODULECLASSES)GetProcAddress(module, "getModuleClasses");
                        if (getModuleClasses)
                        {
                            OutputDebugString(Gek::String::format(L"GEK Plugin Found: %s\r\n", fileName));

                            std::unordered_map<CLSID, std::function<HRESULT(ContextUserInterface **)>> moduleClasses;
                            std::unordered_map<CLSID, std::vector<CLSID>> moduleTypedClasses;

                            if (SUCCEEDED(getModuleClasses(moduleClasses, moduleTypedClasses)))
                            {
                                for (auto &moduleClass : moduleClasses)
                                {
                                    if (classes.find(moduleClass.first) == classes.end())
                                    {
                                        classes[moduleClass.first] = moduleClass.second;
                                        OutputDebugString(Gek::String::format(L"- Adding class from plugin: %s\r\n", CStringW(CComBSTR(moduleClass.first)).GetString()));
                                    }
                                    else
                                    {
                                        OutputDebugString(Gek::String::format(L"! Duplicate class found: %s\r\n", CStringW(CComBSTR(moduleClass.first)).GetString()));
                                    }
                                }

                                for (auto &moduleTypedClass : moduleTypedClasses)
                                {
                                    typedClasses[moduleTypedClass.first].insert(typedClasses[moduleTypedClass.first].end(), moduleTypedClass.second.begin(), moduleTypedClass.second.end());
                                }
                            }
                            else
                            {
                                OutputDebugString(L"! Unable to get class list from module");
                            }
                        }
                    }

                    return S_OK;
                });
            }
        }

        STDMETHODIMP createInstance(REFGUID classID, REFIID interfaceID, LPVOID FAR *newInstance)
        {
            REQUIRE_RETURN(newInstance, E_INVALIDARG);

            HRESULT returnValue = E_FAIL;
            auto classIterator = classes.find(classID);
            if (classIterator != classes.end())
            {
                CComPtr<ContextUserInterface> classInstance;
                returnValue = ((*classIterator).second)(&classInstance);
                if (SUCCEEDED(returnValue) && classInstance)
                {
                    classInstance->registerContext(this);
                    returnValue = classInstance->QueryInterface(interfaceID, newInstance);
                }
            }

            return returnValue;
        }

        STDMETHODIMP createEachType(REFCLSID typeID, std::function<HRESULT(REFCLSID, IUnknown *)> onCreateInstance)
        {
            HRESULT returnValue = S_OK;
            auto typedClassIterator = typedClasses.find(typeID);
            if (typedClassIterator != typedClasses.end())
            {
                for (auto &classID : (*typedClassIterator).second)
                {
                    CComPtr<IUnknown> classInstance;
                    returnValue = createInstance(classID, IID_PPV_ARGS(&classInstance));
                    if (classInstance)
                    {
                        returnValue = onCreateInstance(classID, classInstance);
                        if (FAILED(returnValue))
                        {
                            break;
                        }
                    }
                };
            }

            return returnValue;
        }
    };

    HRESULT createContext(ContextInterface **output)
    {
        REQUIRE_RETURN(output, E_INVALIDARG);

        HRESULT returnValue = E_OUTOFMEMORY;
        CComPtr<Context> context(new Context());
        _ASSERTE(context);
        if (context)
        {
            returnValue = context->QueryInterface(IID_PPV_ARGS(output));
        }

        return returnValue;
    }
};
