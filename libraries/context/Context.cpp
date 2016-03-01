#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\Common.h"
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
    LoggingScope::LoggingScope(Context *context, HRESULT *returnValue, LPCSTR file, UINT32 line, const CStringW &call)
        : context(context)
        , returnValue(returnValue)
        , file(file)
        , line(line)
        , call(call)
        , startTime(GetTickCount())
    {
        context->logMessage(file, line, 1, L"[entering] %s", call.GetString());
    }

    LoggingScope::~LoggingScope(void)
    {
        UINT32 totalTime(GetTickCount() - startTime);
        if (returnValue)
        {
            context->logMessage(file, line, -1, L"[leaving] %s: %s (0x%08X) (%ums)", call.GetString(), (SUCCEEDED(*returnValue) ? L"succeeded" : L"failed"), (*returnValue), totalTime);
        }
        else
        {
            context->logMessage(file, line, -1, L"[leaving] %s (%ums)", call.GetString(), totalTime);
        }
    }

    class ContextImplementation : virtual public UnknownMixin
        , virtual public ObservableMixin
        , virtual public Context
    {
    private:
        std::list<CStringW> searchPathList;

        std::list<HMODULE> moduleList;
        std::unordered_map<CLSID, std::function<HRESULT(ContextUser **)>> classList;
        std::unordered_map<CLSID, std::vector<CLSID>> typedClassList;

        UINT32 nextListenerHandle;
        std::unordered_map<UINT32, std::function<void(LPVOID)>> listenerMap;
        std::unordered_multimap<std::type_index, UINT32> eventListenerMap;

        long loggingIndent;

    public:
        ContextImplementation(void)
            : loggingIndent(0)
            , nextListenerHandle(0)
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

        // Context
        STDMETHODIMP_(void) addSearchPath(LPCWSTR fileName)
        {
            searchPathList.push_back(fileName);
        }

        STDMETHODIMP_(void) initialize(void)
        {
            logMessage(__FILE__, __LINE__, 1, L"[entering] %S...", __FUNCTION__);

            searchPathList.push_back(L"%root%");
            for (auto &searchPath : searchPathList)
            {
                Gek::FileSystem::find(searchPath, L"*.dll", false, [&](LPCWSTR fileName) -> HRESULT
                {
                    HMODULE module = LoadLibrary(fileName);
                    if (module)
                    {
                        typedef HRESULT(*GEKGETMODULECLASSES)(std::unordered_map<CLSID, std::function<HRESULT(ContextUser **)>> &, std::unordered_map<CLSID, std::vector<CLSID >> &);
                        GEKGETMODULECLASSES getModuleClasses = (GEKGETMODULECLASSES)GetProcAddress(module, "GEKGetModuleClasses");
                        if (getModuleClasses)
                        {
                            logMessage(__FILE__, __LINE__, 0, L"GEK Plugin Found: %s", fileName);

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
                                        logMessage(__FILE__, __LINE__, 0, L"Adding class from plugin: %s", CStringW(CComBSTR(moduleClass.first)).GetString());
                                    }
                                    else
                                    {
                                        logMessage(__FILE__, __LINE__, 0, L"[error] Duplicate class found: %s", CStringW(CComBSTR(moduleClass.first)).GetString());
                                    }
                                }

                                for (auto &moduleTypedClass : moduleTypedClassList)
                                {
                                    typedClassList[moduleTypedClass.first].insert(typedClassList[moduleTypedClass.first].end(), moduleTypedClass.second.begin(), moduleTypedClass.second.end());
                                }
                            }
                            else
                            {
                                logMessage(__FILE__, __LINE__, 0, L"[error] Unable to get class list from module");
                            }
                        }
                    }
                    else
                    {
                        DWORD errorCode = GetLastError();
                        logMessage(__FILE__, __LINE__, 0, L"[error] Unable to load library: 0x%08X", errorCode);
                    }

                    return S_OK;
                });
            }

            logMessage(__FILE__, __LINE__, -1, L"[leaving] %S", __FUNCTION__);
        }

        STDMETHODIMP createInstance(REFGUID className, REFIID interfaceType, LPVOID FAR *returnObject)
        {
            REQUIRE_RETURN(returnObject, E_INVALIDARG);

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
                    if (FAILED(resultValue))
                    {
                        logMessage(__FILE__, __LINE__, 0, L"Class doesn't support interface: %s (%s)", CStringW(CComBSTR(className)).GetString(), CStringW(CComBSTR(interfaceType)).GetString());
                    }
                }
                else
                {
                    logMessage(__FILE__, __LINE__, 0, L"Unable to create class: %s (0x%08X)", CStringW(CComBSTR(className)).GetString(), resultValue);
                }
            }
            else
            {
                logMessage(__FILE__, __LINE__, 0, L"Unable to locate class: %s", CStringW(CComBSTR(className)).GetString());
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
            else
            {
                logMessage(__FILE__, __LINE__, 0, L"Unable to locate class type: %s", CStringW(CComBSTR(typeName)).GetString());
            }

            return resultValue;
        }

        STDMETHODIMP_(UINT32) addListener(std::type_index type, std::function<void(LPVOID)> onEvent)
        {
            UINT32 listenerHandle = InterlockedIncrement(&nextListenerHandle);
            listenerMap[listenerHandle] = onEvent;
            eventListenerMap.insert(std::make_pair(type, listenerHandle));
            return listenerHandle;
        }

        STDMETHODIMP_(void) removeListener(UINT32 listenerHandle)
        {
            listenerMap.erase(listenerHandle);
            auto eventIterator = std::find_if(eventListenerMap.begin(), eventListenerMap.end(), [listenerHandle](std::pair<const std::type_index, UINT32> &eventPair) -> bool
            {
                return (eventPair.second == listenerHandle);
            });

            if (eventIterator != eventListenerMap.end())
            {
                eventListenerMap.erase(eventIterator);
            }
        }

        STDMETHODIMP_(void) sendEvent(const std::type_index &type, LPVOID data)
        {
            auto groupRange = eventListenerMap.equal_range(type);
            for (auto listenerKey = groupRange.first; listenerKey != groupRange.second; listenerKey++)
            {
                listenerMap[listenerKey->second](data);
            }
        }

        STDMETHODIMP_(void) logMessage(LPCSTR file, UINT32 line, INT32 changeIndent, LPCWSTR format, ...)
        {
            if (format != nullptr)
            {
                CStringW message;

                va_list variableList;
                va_start(variableList, format);
                message.FormatV(format, variableList);
                va_end(variableList);

                std::vector<wchar_t> indent;
                if (changeIndent < 0)
                {
                    InterlockedAdd(&loggingIndent, changeIndent);

                    indent.push_back(L'<');
                    std::vector<wchar_t> bar(loggingIndent * 2, L'-');
                    indent.insert(indent.end(), bar.begin(), bar.end());
                    indent.push_back(L'<');
                    indent.push_back(L' ');
                }
                else if (changeIndent > 0)
                {
                    indent.push_back(L'>');
                    std::vector<wchar_t> bar(loggingIndent * 2, L'-');
                    indent.insert(indent.end(), bar.begin(), bar.end());
                    indent.push_back(L'>');
                    indent.push_back(L' ');

                    InterlockedAdd(&loggingIndent, changeIndent);
                }
                else if (changeIndent == 0)
                {
                    std::vector<wchar_t> bar(loggingIndent * 2, L' ');
                    indent.insert(indent.end(), bar.begin(), bar.end());
                }

                indent.push_back(L'\0');

                message = (indent.data() + message);
                OutputDebugString(Gek::String::format(L"% 30S (%05d)%s\r\n", file, line, message.GetString()));

                MessageEvent messageEvent;
                messageEvent.file = file;
                messageEvent.line = line;
                messageEvent.message = message;
                Context::sendEvent(messageEvent);
                ObservableMixin::sendEvent(Event<ContextObserver>(std::bind(&ContextObserver::onLogMessage, std::placeholders::_1, file, line, message.GetString())));
            }
        }
    };

    HRESULT Context::create(Context **returnObject)
    {
        REQUIRE_RETURN(returnObject, E_INVALIDARG);

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
