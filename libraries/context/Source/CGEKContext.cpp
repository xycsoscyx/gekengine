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
    if (spContext)
    {
        hRetVal = spContext->QueryInterface(IID_PPV_ARGS(ppContext));
    }

    return hRetVal;
}

CGEKContext::CGEKContext(void)
{
}

CGEKContext::~CGEKContext(void)
{
    m_aClasses.clear();
    m_aTypedClasses.clear();
    for (auto &hModule : m_aModules)
    {
        FreeLibrary(hModule);
    }
}

STDMETHODIMP_(ULONG) CGEKContext::AddRef(void)
{
    return InterlockedIncrement(&m_nRefCount);
}

STDMETHODIMP_(ULONG) CGEKContext::Release(void)
{
    LONG nRefCount = InterlockedDecrement(&m_nRefCount);
    if (nRefCount == 0)
    {
        delete this;
    }

    return nRefCount;
}

STDMETHODIMP CGEKContext::QueryInterface(REFIID rIID, LPVOID FAR *ppObject)
{
    REQUIRE_RETURN(ppObject, E_INVALIDARG);

    HRESULT hRetVal = E_INVALIDARG;
    if (IsEqualIID(IID_IUnknown, rIID))
    {
        AddRef();
        (*ppObject) = dynamic_cast<IUnknown *>(dynamic_cast<IGEKContext *>(this));
        _ASSERTE(*ppObject);
        hRetVal = S_OK;
    }
    else if (IsEqualIID(__uuidof(IGEKUnknown), rIID))
    {
        AddRef();
        (*ppObject) = dynamic_cast<IGEKUnknown *>(this);
        _ASSERTE(*ppObject);
        hRetVal = S_OK;
    }
    else if (IsEqualIID(__uuidof(IGEKContext), rIID))
    {
        AddRef();
        (*ppObject) = dynamic_cast<IGEKContext *>(this);
        _ASSERTE(*ppObject);
        hRetVal = S_OK;
    }

    return hRetVal;
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
                typedef HRESULT(*GEKGETMODULECLASSES)(std::unordered_map<CLSID, std::function<HRESULT(IGEKUnknown **)>> &, 
                                                      std::unordered_map<CLSID, 
                                                      std::vector<CLSID>> &);
                GEKGETMODULECLASSES GEKGetModuleClasses = (GEKGETMODULECLASSES)GetProcAddress(hModule, "GEKGetModuleClasses");
                if (GEKGetModuleClasses)
                {
                    OutputDebugString(FormatString(L"GEK Plugin Found: %s\r\n", pFileName));
                    std::unordered_map<CLSID, std::function<HRESULT(IGEKUnknown **ppObject)>> aClasses;
                    std::unordered_map<CLSID, std::vector<CLSID>> aTypedClasses;
                    if (SUCCEEDED(GEKGetModuleClasses(aClasses, aTypedClasses)))
                    {
                        for (auto &kPair : aClasses)
                        {
                            CComBSTR spClass(kPair.first);
                            CStringW strClass(spClass);

                            if (m_aClasses.find(kPair.first) == m_aClasses.end())
                            {
                                m_aClasses[kPair.first] = kPair.second;
                                OutputDebugString(FormatString(L"- Adding class from plugin: %s\r\n", strClass.GetString()));
                            }
                            else
                            {
                                OutputDebugString(FormatString(L"! Duplicate class found: %s\r\n", strClass.GetString()));
                            }
                        }

                        for (auto &kPair : aTypedClasses)
                        {
                            m_aTypedClasses[kPair.first].insert(m_aTypedClasses[kPair.first].end(), kPair.second.begin(), kPair.second.end());
                        }
                    }
                    else
                    {
                        OutputDebugString(L"! Unable to get class list from module");
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
        CComPtr<IGEKUnknown> spObject;
        hRetVal = ((*pIterator).second)(&spObject);
        if (SUCCEEDED(hRetVal) && spObject)
        {
            spObject->RegisterContext(this);
            hRetVal = spObject->QueryInterface(kIID, ppObject);
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKContext::CreateEachType(REFCLSID kTypeCLSID, std::function<HRESULT(IUnknown *pObject)> OnCreate)
{
    HRESULT hRetVal = S_OK;
    auto pIterator = m_aTypedClasses.find(kTypeCLSID);
    if (pIterator != m_aTypedClasses.end())
    {
        for (auto &kCLSID : (*pIterator).second)
        {
            CComPtr<IUnknown> spObject;
            hRetVal = CreateInstance(kCLSID, IID_PPV_ARGS(&spObject));
            if (spObject)
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
