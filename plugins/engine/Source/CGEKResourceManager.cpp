#include "CGEKResourceManager.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKResourceManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKResourceManager)

CGEKResourceManager::CGEKResourceManager(void)
    : m_pSystem(nullptr)
    , m_pVideoSystem(nullptr)
{
}

CGEKResourceManager::~CGEKResourceManager(void)
{
}

STDMETHODIMP CGEKResourceManager::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = E_FAIL;
    m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    if (m_pSystem != nullptr && m_pVideoSystem != nullptr)
    {
        m_spThread.reset(new std::thread([&](void)-> void
        {
            while (true)
            {
                REQUEST kRequest;
                if (m_aQueue.try_pop(kRequest))
                {
                    concurrency::critical_section::scoped_lock kLock(m_kCritical);

                    GEKHASH nID(kRequest.m_strName + L"|" + kRequest.m_strParams);

                    m_aResources[nID] = nullptr;

                    auto pRange = m_aCallbacks.equal_range(nID);
                    auto pIterator = pRange.first;
                    while(pIterator != pRange.second)
                    {
                        (*pIterator).second(nullptr);
                        m_aCallbacks.unsafe_erase(++pIterator);
                    };
                }
            };
        }));

        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKResourceManager::Destroy(void)
{
}

STDMETHODIMP CGEKResourceManager::Load(LPCWSTR pName, LPCWSTR pParams, std::function<void(IUnknown *)> OnReady)
{
    concurrency::critical_section::scoped_lock kLock(m_kCritical);

    GEKHASH nID = FormatString(L"%s|%s", pName, pParams);
    auto pIterator = m_aResources.find(nID);
    if (pIterator != m_aResources.end())
    {
        OnReady((*pIterator).second);
    }
    else
    {
        if (m_aCallbacks.find(nID) == m_aCallbacks.end())
        {
            m_aQueue.push(REQUEST(pName, pParams));
        }

        m_aCallbacks.insert(std::make_pair(nID, OnReady));
    }

    return S_OK;
}
