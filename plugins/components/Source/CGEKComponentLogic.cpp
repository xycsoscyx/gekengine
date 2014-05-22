#include "CGEKComponentLogic.h"
#include <algorithm>
#include <ppl.h>

#include "GEKComponentsCLSIDs.h"
#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentLogic)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentLogic::CGEKComponentLogic(IGEKContext *pContext, IGEKEntity *pEntity)
    : CGEKUnknown(pContext)
    , CGEKComponent(pEntity)
{
}

CGEKComponentLogic::~CGEKComponentLogic(void)
{
    m_spState = nullptr;
}

void CGEKComponentLogic::SetState(IGEKLogicState *pState)
{
    if (m_spState)
    {
        m_spState->OnExit();
    }

    m_spState = pState;
    if (m_spState)
    {
        m_spState->OnEnter(GetEntity());
    }
}

STDMETHODIMP_(void) CGEKComponentLogic::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"state", m_strDefaultState.GetString());
}

static GEKHASH gs_nState(L"state");
STDMETHODIMP_(bool) CGEKComponentLogic::GetProperty(LPCWSTR pName, GEKVALUE &kValue) const
{
    GEKHASH nHash(pName);
    if (nHash == gs_nState)
    {
        kValue = m_strDefaultState.GetString();
        return true;
    }

    return false;
}

STDMETHODIMP_(bool) CGEKComponentLogic::SetProperty(LPCWSTR pName, const GEKVALUE &kValue)
{
    GEKHASH nHash(pName);
    if (nHash == gs_nState)
    {
        m_strDefaultState = kValue.GetString();
        return true;
    }

    return false;
}

STDMETHODIMP CGEKComponentLogic::OnEntityCreated(void)
{
    HRESULT hRetVal = S_OK;
    if (!m_strDefaultState.IsEmpty())
    {
        CComPtr<IGEKLogicState> spState;
        hRetVal = GetContext()->CreateNamedInstance(m_strDefaultState, IID_PPV_ARGS(&spState));
        if (spState)
        {
            SetState(spState);
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentLogic::OnEvent(LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB)
{
    if (m_spState)
    {
        m_spState->OnEvent(pAction, kParamA, kParamB);
    }
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemLogic)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemLogic)

CGEKComponentSystemLogic::CGEKComponentSystemLogic(void)
{
}

CGEKComponentSystemLogic::~CGEKComponentSystemLogic(void)
{
}

STDMETHODIMP CGEKComponentSystemLogic::Initialize(void)
{
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKComponentSystemLogic, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->AddCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemLogic::Destroy(void)
{
    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
    GetContext()->RemoveCachedClass(CLSID_GEKComponentSystemLogic);
}

STDMETHODIMP_(LPCWSTR) CGEKComponentSystemLogic::GetType(void) const
{
    return L"logic";
}

STDMETHODIMP_(void) CGEKComponentSystemLogic::Clear(void)
{
    m_aComponents.clear();
}

STDMETHODIMP CGEKComponentSystemLogic::Create(const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_OUTOFMEMORY;
    CComPtr<CGEKComponentLogic> spComponent(new CGEKComponentLogic(GetContext(), pEntity));
    GEKRESULT(spComponent, L"Call to new failed to allocate instance");
    if (spComponent)
    {
        hRetVal = spComponent->QueryInterface(IID_PPV_ARGS(ppComponent));
        if (SUCCEEDED(hRetVal))
        {
            kComponentNode.ListAttributes([&spComponent](LPCWSTR pName, LPCWSTR pValue) -> void
            {
                spComponent->SetProperty(pName, pValue);
            });

            m_aComponents[pEntity] = spComponent;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemLogic::Destroy(IGEKEntity *pEntity)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aComponents.find(pEntity);
    if (pIterator != m_aComponents.end())
    {
        m_aComponents.unsafe_erase(pIterator);
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemLogic::OnPreUpdate(float nGameTime, float nFrameTime)
{
    concurrency::parallel_for_each(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentLogic>>::value_type &kPair) -> void
    {
        if (kPair.second->m_spState)
        {
            kPair.second->m_spState->OnUpdate(nGameTime, nFrameTime);
        }
    });
}

STDMETHODIMP_(void) CGEKComponentSystemLogic::SetState(IGEKEntity *pEntity, IGEKLogicState *pState)
{
    auto pIterator = m_aComponents.find(pEntity);
    if (pIterator != m_aComponents.end())
    {
        (*pIterator).second->SetState(pState);
    }
}
