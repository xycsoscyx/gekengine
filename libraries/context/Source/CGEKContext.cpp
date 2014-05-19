#include "CGEKContext.h"
#include <algorithm>
#include <atlpath.h>

#include <initguid.h>

HRESULT GEKCreateContext(IGEKContext **ppContext)
{
    REQUIRE_RETURN(ppContext, E_INVALIDARG);

    HRESULT hRetVal = E_OUTOFMEMORY;
    CComPtr<CGEKContext> spContext(new CGEKContext());
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
{
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
                typedef HRESULT(*GEKGETMODULECLASSES)(std::map<CLSID, std::function<HRESULT(IGEKUnknown **)>, CCompareGUID> &, std::map<CStringW, CLSID> &, std::map<CLSID, std::vector<CLSID>> &);
                GEKGETMODULECLASSES GEKGetModuleClasses = (GEKGETMODULECLASSES)GetProcAddress(hModule, "GEKGetModuleClasses");
                if (GEKGetModuleClasses)
                {
                    std::map<CLSID, std::function<HRESULT (IGEKUnknown **ppObject)>, CCompareGUID> aClasses;
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

STDMETHODIMP CGEKContext::AddCacheClass(REFCLSID kCLSID, IGEKUnknown *pObject)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aCache.find(kCLSID);
    if (pIterator == m_aCache.end())
    {
        m_aCache[kCLSID] = pObject;
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::RemoveCacheClass(REFCLSID kCLSID)
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

STDMETHODIMP CGEKContext::CreateInstance(REFGUID kCLSID, REFIID kIID, LPVOID FAR *ppObject)
{
    REQUIRE_RETURN(ppObject, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pCacheIterator = m_aCache.find(kCLSID);
    if (pCacheIterator != m_aCache.end())
    {
        if ((*pCacheIterator).second)
        {
            hRetVal = (*pCacheIterator).second->QueryInterface(kIID, ppObject);
        }
    }

    if (FAILED(hRetVal))
    {
        auto pIterator = m_aClasses.find(kCLSID);
        if (pIterator != m_aClasses.end())
        {
            CComPtr<IGEKUnknown> spObject;
            hRetVal = ((*pIterator).second)(&spObject);
            if (SUCCEEDED(hRetVal) && spObject)
            {
                hRetVal = spObject->RegisterContext(this);
                if (SUCCEEDED(hRetVal))
                {
                    if (pCacheIterator != m_aCache.end() && !(*pCacheIterator).second)
                    {
                        m_aCache[kCLSID] = spObject;
                    }

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
    if (pIterator != m_aNamedClasses.end())
    {
        hRetVal = CreateInstance(((*pIterator).second), kIID, ppObject);
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::CreateEachType(REFCLSID kTypeCLSID, std::function<HRESULT(IGEKUnknown *pObject)> OnCreate)
{
    HRESULT hRetVal = S_OK;
    auto pIterator = m_aTypedClasses.find(kTypeCLSID);
    if (pIterator != m_aTypedClasses.end())
    {
        for (auto &kCLSID : (*pIterator).second)
        {
            CComPtr<IGEKUnknown> spObject;
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
