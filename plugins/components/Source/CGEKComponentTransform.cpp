#include "CGEKComponentTransform.h"

BEGIN_INTERFACE_LIST(CGEKComponentTransform)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentTransform::CGEKComponentTransform(IGEKEntity *pEntity)
    : CGEKComponent(pEntity)
{
}

CGEKComponentTransform::~CGEKComponentTransform(void)
{
}

STDMETHODIMP_(void) CGEKComponentTransform::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"position", m_nPosition);
    OnProperty(L"rotation", m_nRotation);
}

static GEKHASH gs_nPosition(L"position");
static GEKHASH gs_nRotation(L"rotation");
STDMETHODIMP_(bool) CGEKComponentTransform::GetProperty(LPCWSTR pName, GEKVALUE &kValue) const
{
    GEKHASH nHash(pName);
    if (nHash == gs_nPosition)
    {
        kValue = m_nPosition;
        return true;
    }
    else if (nHash == gs_nRotation)
    {
        kValue = m_nRotation;
        return true;
    }

    return false;
}

STDMETHODIMP_(bool) CGEKComponentTransform::SetProperty(LPCWSTR pName, const GEKVALUE &kValue)
{
    GEKHASH nHash(pName);
    if (nHash == gs_nPosition)
    {
        m_nPosition = kValue.GetFloat3();
        return true;
    }
    else if (nHash == gs_nRotation)
    {
        m_nRotation = kValue.GetQuaternion();
        return true;
    }

    return false;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemTransform)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemTransform)

CGEKComponentSystemTransform::CGEKComponentSystemTransform(void)
{
}

CGEKComponentSystemTransform::~CGEKComponentSystemTransform(void)
{
}

STDMETHODIMP CGEKComponentSystemTransform::Initialize(void)
{
    return S_OK;
}

STDMETHODIMP_(LPCWSTR) CGEKComponentSystemTransform::GetType(void) const
{
    return L"transform";
}

STDMETHODIMP_(void) CGEKComponentSystemTransform::Clear(void)
{
    m_aComponents.clear();
}

STDMETHODIMP CGEKComponentSystemTransform::Create(const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_OUTOFMEMORY;
    CComPtr<CGEKComponentTransform> spComponent(new CGEKComponentTransform(pEntity));
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
            kComponentNode.ListAttributes([&spComponent](LPCWSTR pName, LPCWSTR pValue) -> void
            {
                spComponent->SetProperty(pName, pValue);
            });

            m_aComponents[pEntity] = spComponent;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemTransform::Destroy(IGEKEntity *pEntity)
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
