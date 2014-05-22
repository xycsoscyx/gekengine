#include "CGEKContext.h"
#include <algorithm>
#include <atlpath.h>

#include <initguid.h>

HRESULT GEKCreateContext(IGEKContext **ppContext)
{
    REQUIRE_RETURN(ppContext, E_INVALIDARG);

    HRESULT hRetVal = E_OUTOFMEMORY;
    CComPtr<CGEKContext> spContext(new CGEKContext());
    _ASSERTE(spContext);
    if (spContext != nullptr)
    {
        hRetVal = spContext->QueryInterface(IID_PPV_ARGS(ppContext));
    }

    return hRetVal;
}

BEGIN_INTERFACE_LIST(CGEKContext)
    INTERFACE_LIST_ENTRY_COM(IGEKContext)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
END_INTERFACE_LIST_UNKNOWN

CGEKContext::CGEKContext(void)
    : m_nFrequency(0.0)
    , m_nIndent(0)
{
    UINT64 nFrequency = 0;
    QueryPerformanceFrequency((LARGE_INTEGER *)&nFrequency);
    m_nFrequency = double(nFrequency);
}

CGEKContext::~CGEKContext(void)
{
    m_aClasses.clear();
    m_aNamedClasses.clear();
    m_aTypedClasses.clear();
    for (auto &hModule : m_aModules)
    {
        FreeLibrary(hModule);
    }
}

STDMETHODIMP_(double) CGEKContext::GetTime(void)
{
    UINT64 nCounter = 0;
    QueryPerformanceCounter((LARGE_INTEGER *)&nCounter);
    return (double(nCounter) / m_nFrequency);
}

STDMETHODIMP_(void) CGEKContext::Log(LPCSTR pFile, UINT32 nLine, LPCWSTR pMessage, ...)
{
    CStringW strMessage;
    if (pMessage)
    {
        if (m_nIndent > 0)
        {
            CStringW strIndent;
            strIndent.Format(L"%d", m_nIndent);
            strMessage.Format(L"%" + strIndent + "s", L" ");
        }

        va_list pArgs;
        va_start(pArgs, pMessage);
        strMessage.AppendFormatV(pMessage, pArgs);
        va_end(pArgs);

        if (!strMessage.IsEmpty())
        {
            CGEKObservable::SendEvent(TGEKEvent<IGEKContextObserver>(std::bind(&IGEKContextObserver::OnLog, std::placeholders::_1, pFile, nLine, strMessage.GetString())));
        }
    }
}

STDMETHODIMP_(void) CGEKContext::ChangeIndent(bool bIndent)
{
    m_nIndent += (bIndent ? 1 : -1);
}

STDMETHODIMP CGEKContext::AddSearchPath(LPCWSTR pPath)
{
    m_aSearchPaths.push_back(pPath);
    return S_OK;
}

STDMETHODIMP CGEKContext::Initialize(void)
{
    m_aSearchPaths.push_back(L"%root%");
    for (auto &strPath : m_aSearchPaths)
    {
        GEKFindFiles(strPath, L"*.dll", false, [&] (LPCWSTR pFileName) -> HRESULT
        {
            HMODULE hModule = LoadLibrary(pFileName);
            if (hModule)
            {
                typedef HRESULT (*GEKGETMODULECLASSES)(std::map<CLSID, std::function<HRESULT (IGEKUnknown **)>> &, std::map<CStringW, CLSID> &, std::map<CLSID, std::vector<CLSID>> &);
                GEKGETMODULECLASSES GEKGetModuleClasses = (GEKGETMODULECLASSES)GetProcAddress(hModule, "GEKGetModuleClasses");
                if (GEKGetModuleClasses)
                {
                    std::map<CLSID, std::function<HRESULT (IGEKUnknown **ppObject)>> aClasses;
                    std::map<CStringW, CLSID> aNamedClasses;
                    std::map<CLSID, std::vector<CLSID>> aTypedClasses;
                    if (SUCCEEDED(GEKGetModuleClasses(aClasses, aNamedClasses, aTypedClasses)))
                    {
                        for (auto &kPair : aClasses)
                        {
                            if (m_aClasses.find(kPair.first) == m_aClasses.end())
                            {
                                m_aClasses[kPair.first] = kPair.second;
                            }
                            else
                            {
                                _ASSERTE(!"Duplicate class found while loading plugins");
                            }
                        }

                        for (auto &kPair : aNamedClasses)
                        {
                            if (m_aNamedClasses.find(kPair.first) == m_aNamedClasses.end())
                            {
                                m_aNamedClasses[kPair.first] = kPair.second;
                            }
                            else
                            {
                                _ASSERTE(!"Duplicate name found while loading plugins");
                            }
                        }

                        for (auto &kPair : aTypedClasses)
                        {
                            m_aTypedClasses[kPair.first].insert(m_aTypedClasses[kPair.first].end(), kPair.second.begin(), kPair.second.end());
                        }
                    }
                    else
                    {
                        _ASSERTE(!"Unable to get class list from module");
                    }
                }
            }

            return S_OK;
        } );
    }

    return S_OK;
}

STDMETHODIMP CGEKContext::CreateInstance(REFGUID kCLSID, REFIID kIID, LPVOID FAR *ppObject)
{
    REQUIRE_RETURN(ppObject, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pCacheIterator = m_aCache.find(kCLSID);
    if (pCacheIterator != m_aCache.end())
    {
        hRetVal = (*pCacheIterator).second->QueryInterface(kIID, ppObject);
    }
    else
    {
        auto pIterator = m_aClasses.find(kCLSID);
        GEKRESULT(pIterator != m_aClasses.end(), L"CLSID not registered with context: %s", CStringW(CComBSTR(kCLSID)).GetString());
        if (pIterator != m_aClasses.end())
        {
            CComPtr<IGEKUnknown> spObject;
            hRetVal = ((*pIterator).second)(&spObject);
            if (SUCCEEDED(hRetVal) && spObject)
            {
                hRetVal = spObject->RegisterContext(this);
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = spObject->QueryInterface(kIID, ppObject);
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::CreateNamedInstance(LPCWSTR pName, REFIID kIID, LPVOID FAR *ppObject)
{
    REQUIRE_RETURN(ppObject, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aNamedClasses.find(pName);
    GEKRESULT(pIterator != m_aNamedClasses.end(), L"Name not registered with context: %s", pName);
    if (pIterator != m_aNamedClasses.end())
    {
        hRetVal = CreateInstance(((*pIterator).second), kIID, ppObject);
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::CreateEachType(REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate)
{
    HRESULT hRetVal = S_OK;
    auto pIterator = m_aTypedClasses.find(kTypeCLSID);
    GEKRESULT(pIterator != m_aTypedClasses.end(), L"Type not registered with context: %s", CStringW(CComBSTR(kTypeCLSID)).GetString());
    if (pIterator != m_aTypedClasses.end())
    {
        for (auto &kCLSID : (*pIterator).second)
        {
            CComPtr<IUnknown> spObject;
            hRetVal = CreateInstance(kCLSID, IID_PPV_ARGS(&spObject));
            if (spObject != nullptr)
            {
                hRetVal = OnCreate(spObject);
                if (FAILED(hRetVal))
                {
                    break;
                }
            }
        };
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::AddCachedClass(REFCLSID kCLSID, IUnknown * const pObject)
{
    REQUIRE_RETURN(pObject, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aCache.find(kCLSID);
    if (pIterator == m_aCache.end())
    {
        m_aCache.insert(std::make_pair(kCLSID, pObject));
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::RemoveCachedClass(REFCLSID kCLSID)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aCache.find(kCLSID);
    if (pIterator != m_aCache.end())
    {
        m_aCache.erase(pIterator);
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP_(IUnknown *) CGEKContext::GetCachedClass(REFCLSID kCLSID)
{
    auto pIterator = m_aCache.find(kCLSID);
    if (pIterator != m_aCache.end())
    {
        return (*pIterator).second;
    }

    return nullptr;
}

STDMETHODIMP CGEKContext::AddCachedObserver(REFCLSID kCLSID, IGEKObserver *pObserver)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aCache.find(kCLSID);
    if (pIterator != m_aCache.end())
    {
        CComQIPtr<IGEKObservable> spObservable((*pIterator).second);
        if (spObservable)
        {
            hRetVal = spObservable->AddObserver(pObserver);
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::RemoveCachedObserver(REFCLSID kCLSID, IGEKObserver *pObserver)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aCache.find(kCLSID);
    if (pIterator != m_aCache.end())
    {
        CComQIPtr<IGEKObservable> spObservable((*pIterator).second);
        if (spObservable)
        {
            hRetVal = spObservable->RemoveObserver(pObserver);
        }
    }

    return hRetVal;
}
