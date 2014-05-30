#include "CGEKResourceManager.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKResourceManager)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKResourceManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKResourceManager)

CGEKResourceManager::CGEKResourceManager(void)
    : m_pSystem(nullptr)
    , m_pVideoSystem(nullptr)
    , m_hStopEvent(nullptr)
    , m_nSession(0)
{
}

CGEKResourceManager::~CGEKResourceManager(void)
{
}

STDMETHODIMP_(void) CGEKResourceManager::OnLoadBegin(void)
{
    m_nSession++;
}

STDMETHODIMP CGEKResourceManager::OnLoadEnd(HRESULT hRetVal)
{
    if (FAILED(hRetVal))
    {
        m_nSession++;
    }

    return S_OK;
}

STDMETHODIMP CGEKResourceManager::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = E_FAIL;
    m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    m_hStopEvent = CreateEvent(nullptr, true, false, L"GEK_RESOURCE_THREAD");
    if (m_pSystem != nullptr && m_pVideoSystem != nullptr && m_hStopEvent != nullptr)
    {
        m_spThread.reset(new std::thread([&](void)-> void
        {
            while (WaitForSingleObject(m_hStopEvent, 0) == WAIT_TIMEOUT)
            {
                concurrency::critical_section::scoped_lock kLock(m_kCritical);

                CStringW strName;
                if (m_aQueue.try_pop(strName))
                {
                    std::vector<UINT8> aBuffer;
                    CComPtr<IUnknown> spResource;
                    if (SUCCEEDED(GEKLoadFromFile((L"%root%\\data\\" + strName), aBuffer)))
                    {
                        for (auto pProvider : m_aProviders)
                        {
                            pProvider->Load(strName, &aBuffer[0], aBuffer.size(), &spResource);
                            if (spResource)
                            {
                                break;
                            }
                        }
                    }

                    RESOURCE &kResource = m_aResources[strName];
                    kResource.m_nSession = m_nSession;
                    kResource.m_spObject = spResource;

                    auto pRange = m_aCallbacks.equal_range(strName);
                    for (auto pIterator = pRange.first; pIterator != pRange.second; )
                    {
                        (*pIterator).second(spResource);
                        m_aCallbacks.unsafe_erase(++pIterator);
                    }
                }
            };

            m_spThread.release();
        }));

        hRetVal = S_OK;
    }

    if (SUCCEEDED(hRetVal))
    {
        GetContext()->CreateEachType(CLSID_GEKResourceProviderType, [&](IUnknown *pObject) -> HRESULT
        {
            CComQIPtr<IGEKResourceProvider> spProvider(pObject);
            if (spProvider)
            {
                m_aProviders.push_back(spProvider);
            }

            return S_OK;
        });
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKResourceManager::Destroy(void)
{
    SetEvent(m_hStopEvent);
    while (m_spThread)
    {
        Sleep(1000);
    };
}

STDMETHODIMP CGEKResourceManager::Load(LPCWSTR pName, std::function<void(IUnknown *)> OnReady)
{
    concurrency::critical_section::scoped_lock kLock(m_kCritical);
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        (*pIterator).second.m_nSession = m_nSession;
        OnReady((*pIterator).second.m_spObject);
    }
    else
    {
        if (m_aCallbacks.find(pName) == m_aCallbacks.end())
        {
            m_aQueue.push(pName);
        }

        m_aCallbacks.insert(std::make_pair(pName, OnReady));
    }

    return S_OK;
}
