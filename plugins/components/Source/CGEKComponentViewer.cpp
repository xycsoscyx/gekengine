#include "CGEKComponentViewer.h"

BEGIN_INTERFACE_LIST(CGEKComponentViewer)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentViewer::CGEKComponentViewer(IGEKEntity *pEntity)
    : CGEKComponent(pEntity)
    , m_nFieldOfView(0.0f)
    , m_nMinViewDistance(0.0f)
    , m_nMaxViewDistance(0.0f)
{
}

CGEKComponentViewer::~CGEKComponentViewer(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentViewer::GetType(void) const
{
    return L"viewer";
}

STDMETHODIMP_(void) CGEKComponentViewer::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"fieldofview", m_nFieldOfView);
    OnProperty(L"minviewdistance", m_nMinViewDistance);
    OnProperty(L"maxviewdistance", m_nMaxViewDistance);
}

static GEKHASH gs_nFieldOfView(L"fieldofview");
static GEKHASH gs_nMinViewDistance(L"minviewdistance");
static GEKHASH gs_nMaxViewDistance(L"maxviewdistance");
STDMETHODIMP_(bool) CGEKComponentViewer::GetProperty(LPCWSTR pName, GEKVALUE &kValue) const
{
    GEKHASH nHash(pName);
    if (nHash == gs_nFieldOfView)
    {
        kValue = m_nFieldOfView;
        return true;
    }
    else if (nHash == gs_nMinViewDistance)
    {
        kValue = m_nMinViewDistance;
        return true;
    }
    else if (nHash == gs_nMaxViewDistance)
    {
        kValue = m_nMaxViewDistance;
        return true;
    }

    return false;
}

STDMETHODIMP_(bool) CGEKComponentViewer::SetProperty(LPCWSTR pName, const GEKVALUE &kValue)
{
    GEKHASH nHash(pName);
    if (nHash == gs_nFieldOfView)
    {
        m_nFieldOfView = kValue.GetFloat();
        return true;
    }
    else if (nHash == gs_nMinViewDistance)
    {
        m_nMinViewDistance = kValue.GetFloat();
        return true;
    }
    else if (nHash == gs_nMaxViewDistance)
    {
        m_nMaxViewDistance = kValue.GetFloat();
        return true;
    }

    return false;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemViewer)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemViewer)

CGEKComponentSystemViewer::CGEKComponentSystemViewer(void)
{
}

CGEKComponentSystemViewer::~CGEKComponentSystemViewer(void)
{
}

STDMETHODIMP CGEKComponentSystemViewer::Initialize(void)
{
    return S_OK;
}

STDMETHODIMP_(void) CGEKComponentSystemViewer::Clear(void)
{
    m_aComponents.clear();
}

STDMETHODIMP CGEKComponentSystemViewer::Create(const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_FAIL;
    if (kNode.HasAttribute(L"type") && kNode.GetAttribute(L"type").CompareNoCase(L"viewer") == 0)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKComponentViewer> spComponent(new CGEKComponentViewer(pEntity));
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

STDMETHODIMP CGEKComponentSystemViewer::Destroy(IGEKEntity *pEntity)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = std::find_if(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentViewer>>::value_type &kPair) -> bool
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
