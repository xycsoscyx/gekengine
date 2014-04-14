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
                typedef HRESULT (*GEKGETMODULECLASSES)(std::map<CLSID, std::function<HRESULT (IUnknown **)>, CCompareGUID> &, std::map<CStringW, CLSID> &, std::map<CLSID, std::vector<CLSID>> &);
                GEKGETMODULECLASSES GEKGetModuleClasses = (GEKGETMODULECLASSES)GetProcAddress(hModule, "GEKGetModuleClasses");
                if (GEKGetModuleClasses)
                {
                    std::map<CLSID, std::function<HRESULT (IUnknown **ppObject)>, CCompareGUID> aClasses;
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
    auto pIterator = m_aClasses.find(kCLSID);
    if (pIterator != m_aClasses.end())
    {
        CComPtr<IUnknown> spObject;
        hRetVal = ((*pIterator).second)(&spObject);
        if (SUCCEEDED(hRetVal) && spObject)
        {
            hRetVal = RegisterInstance(spObject);
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = spObject->QueryInterface(kIID, ppObject);
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

STDMETHODIMP CGEKContext::CreateEachType(REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate)
{
    HRESULT hRetVal = S_OK;
    auto pIterator = m_aTypedClasses.find(kTypeCLSID);
    if (pIterator != m_aTypedClasses.end())
    {
        std::find_if (((*pIterator).second).begin(), ((*pIterator).second).end(), [&] (REFCLSID kCLSID) -> bool
        {
            CComPtr<IUnknown> spObject;
            hRetVal = CreateInstance(kCLSID, IID_PPV_ARGS(&spObject));
            if (spObject != nullptr)
            {
                hRetVal = OnCreate(spObject);
            }

            return FAILED(hRetVal);
        } );
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::RegisterInstance(IUnknown *pObject)
{
    HRESULT hRetVal = S_OK;
    CComQIPtr<IGEKContextUser> spContextUser(pObject);
    if (spContextUser != nullptr)
    {
        hRetVal = spContextUser->Register(this);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::SendEvent(TGEKEvent<IGEKContextObserver>(std::bind(&IGEKContextObserver::OnRegistration, std::placeholders::_1, pObject)));
    }

    if (SUCCEEDED(hRetVal))
    {
        CComQIPtr<IGEKUnknown> spObject(pObject);
        if (spObject != nullptr)
        {
            hRetVal = spObject->Initialize();
        }
    }

    return hRetVal;
}
