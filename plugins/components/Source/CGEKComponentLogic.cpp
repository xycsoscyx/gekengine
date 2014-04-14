#include "CGEKComponentLogic.h"
#include <luabind/adopt_policy.hpp>
#include <algorithm>

BEGIN_INTERFACE_LIST(CGEKComponentLogic)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentLogic::CGEKComponentLogic(IGEKEntity *pEntity)
    : CGEKComponent(pEntity)
    , m_hModule(nullptr)
{
}

CGEKComponentLogic::~CGEKComponentLogic(void)
{
    if (m_hModule)
    {
        FreeLibrary(m_hModule);
    }
}

CGEKComponentLogic::STATE &CGEKComponentLogic::GetCurrentState(void)
{
    return m_kCurrentState;
}

STDMETHODIMP_(LPCWSTR) CGEKComponentLogic::GetType(void) const
{
    return L"logic";
}

STDMETHODIMP_(void) CGEKComponentLogic::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"module", m_strModule.GetString());
    OnProperty(L"function", m_strFunction.GetString());
}

static GEKHASH gs_nModule(L"module");
static GEKHASH gs_nFunction(L"function");
STDMETHODIMP_(bool) CGEKComponentLogic::GetProperty(LPCWSTR pName, GEKVALUE &kValue) const
{
    GEKHASH nHash(pName);
    if (nHash == gs_nModule)
    {
        kValue = m_strModule.GetString();
        return true;
    }
    else if (nHash == gs_nFunction)
    {
        kValue = m_strFunction.GetString();
        return true;
    }

    return false;
}

STDMETHODIMP_(bool) CGEKComponentLogic::SetProperty(LPCWSTR pName, const GEKVALUE &kValue)
{
    GEKHASH nHash(pName);
    if (nHash == gs_nModule)
    {
        m_strModule = kValue.GetString();
        return true;
    }
    else if (nHash == gs_nFunction)
    {
        m_strFunction = kValue.GetString();
        return true;
    }

    return false;
}

STDMETHODIMP CGEKComponentLogic::OnEntityCreated(void)
{
    HRESULT hRetVal = S_OK;
    if (!m_strModule.IsEmpty() && !m_strFunction.IsEmpty())
    {
        m_hModule = LoadLibrary(m_strModule);
        if (m_hModule)
        {
            typedef HRESULT(*GEINITIALIZELOGIC)(IGEKEntity *,
                std::function<void(IGEKEntity *)>,
                std::function<void(IGEKEntity *)>,
                std::function<void(IGEKEntity *, float, float)>,
                std::function<void(IGEKEntity *, LPCWSTR, const GEKVALUE &, const GEKVALUE &)>);
            GEINITIALIZELOGIC Initialize = (GEINITIALIZELOGIC)GetProcAddress(m_hModule, CW2A(m_strFunction, CP_UTF8));
            if (Initialize)
            {
                hRetVal = Initialize(GetEntity(), m_kCurrentState.OnEnter, m_kCurrentState.OnExit, m_kCurrentState.OnUpdate, m_kCurrentState.OnEvent);
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentLogic::OnEvent(LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB)
{
    if (m_kCurrentState.OnEvent)
    {
        m_kCurrentState.OnEvent(GetEntity(), pAction, kParamA, kParamB);
    }

    return S_OK;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemLogic)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManagerUser)
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
    return CGEKObservable::AddObserver(GetSceneManager(), this);
}

STDMETHODIMP_(void) CGEKComponentSystemLogic::Destroy(void)
{
    CGEKObservable::RemoveObserver(GetSceneManager(), this);
}

STDMETHODIMP_(void) CGEKComponentSystemLogic::Clear(void)
{
    m_aComponents.clear();
}

STDMETHODIMP CGEKComponentSystemLogic::Create(const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_FAIL;
    if (kNode.HasAttribute(L"type") && kNode.GetAttribute(L"type").CompareNoCase(L"logic") == 0)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKComponentLogic> spComponent(new CGEKComponentLogic(pEntity));
        if (spComponent)
        {
            CComPtr<IUnknown> spComponentUnknown;
            spComponent->QueryInterface(IID_PPV_ARGS(&spComponentUnknown));
            if (spComponentUnknown)
            {
                GetContext()->RegisterInstance(spComponentUnknown);
            }

            hRetVal = spComponent->QueryInterface(IID_PPV_ARGS(ppComponent));
            if (SUCCEEDED(hRetVal))
            {
                kNode.ListAttributes([&spComponent] (LPCWSTR pName, LPCWSTR pValue) -> void
                {
                    spComponent->SetProperty(pName, pValue);
                } );

                m_aComponents[pEntity] = spComponent;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemLogic::Destroy(IGEKEntity *pEntity)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = std::find_if(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentLogic>>::value_type &kPair) -> bool
    {
        return (kPair.first == pEntity);
    });

    if (pIterator != m_aComponents.end())
    {
        m_aComponents.erase(pIterator);
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemLogic::OnPreUpdate(float nGameTime, float nFrameTime)
{
    for (auto &kPair : m_aComponents)
    {
        if (kPair.second->GetCurrentState().OnUpdate)
        {
            kPair.second->GetCurrentState().OnUpdate(kPair.first, nGameTime, nFrameTime);
        }
    }

    return S_OK;
}