#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\ContextInterface.h"
#include "GEK\Context\ContextUserInterface.h"
#include "GEK\Context\Observable.h"
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

        std::list<HMODULE> moduleList;
        std::unordered_map<CLSID, std::function<HRESULT(ContextUserInterface **)>> classList;
        std::unordered_map<CLSID, std::vector<CLSID>> typedClassList;

    public:
        ~Context(void)
        {
            classList.clear();
            typedClassList.clear();
            for (auto &module : moduleList)
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
        STDMETHODIMP_(void) addSearchPath(LPCWSTR fileName)
        {
            searchPaths.push_back(fileName);
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

                            moduleList.push_back(module);
                            std::unordered_map<CLSID, std::function<HRESULT(ContextUserInterface **)>> moduleClassList;
                            std::unordered_map<CLSID, std::vector<CLSID>> moduleTypedClassList;

                            if (SUCCEEDED(getModuleClasses(moduleClassList, moduleTypedClassList)))
                            {
                                for (auto &moduleClass : moduleClassList)
                                {
                                    if (classList.find(moduleClass.first) == classList.end())
                                    {
                                        classList[moduleClass.first] = moduleClass.second;
                                        OutputDebugString(Gek::String::format(L"- Adding class from plugin: %s\r\n", CStringW(CComBSTR(moduleClass.first)).GetString()));
                                    }
                                    else
                                    {
                                        OutputDebugString(Gek::String::format(L"! Duplicate class found: %s\r\n", CStringW(CComBSTR(moduleClass.first)).GetString()));
                                    }
                                }

                                for (auto &moduleTypedClass : moduleTypedClassList)
                                {
                                    typedClassList[moduleTypedClass.first].insert(typedClassList[moduleTypedClass.first].end(), moduleTypedClass.second.begin(), moduleTypedClass.second.end());
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
            auto classIterator = classList.find(classID);
            if (classIterator != classList.end())
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
            auto typedClassIterator = typedClassList.find(typeID);
            if (typedClassIterator != typedClassList.end())
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
