#include "CGEKComponentLight.h"
#include <algorithm>
#include <ppl.h>

BEGIN_INTERFACE_LIST(CGEKComponentLight)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentLight::CGEKComponentLight(IGEKContext *pContext, IGEKEntity *pEntity)
    : CGEKUnknown(pContext)
    , CGEKComponent(pEntity)
    , m_nRange(0.0f)
{
}

CGEKComponentLight::~CGEKComponentLight(void)
{
}

STDMETHODIMP_(void) CGEKComponentLight::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"color", m_nColor);
    OnProperty(L"range", m_nRange);
}

static GEKHASH gs_nColor(L"color");
static GEKHASH gs_nRange(L"range");
STDMETHODIMP_(bool) CGEKComponentLight::GetProperty(LPCWSTR pName, GEKVALUE &kValue) const
{
    GEKHASH nHash(pName);
    if (nHash == gs_nColor)
    {
        kValue = m_nColor;
        return true;
    }
    else if (nHash == gs_nRange)
    {
        kValue = m_nRange;
        return true;
    }

    return false;
}

STDMETHODIMP_(bool) CGEKComponentLight::SetProperty(LPCWSTR pName, const GEKVALUE &kValue)
{
    GEKHASH nHash(pName);
    if (nHash == gs_nColor)
    {
        m_nColor = kValue.GetFloat3();
        return true;
    }
    else if (nHash == gs_nRange)
    {
        m_nRange = kValue.GetFloat();
        return true;
    }

    return false;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemLight)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemLight)

CGEKComponentSystemLight::CGEKComponentSystemLight(void)
{
}

CGEKComponentSystemLight::~CGEKComponentSystemLight(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentSystemLight::GetType(void) const
{
    return L"light";
}

STDMETHODIMP_(void) CGEKComponentSystemLight::Clear(void)
{
    m_aComponents.clear();
}

STDMETHODIMP CGEKComponentSystemLight::Create(const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_OUTOFMEMORY;
    CComPtr<CGEKComponentLight> spComponent(new CGEKComponentLight(GetContext(), pEntity));
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

STDMETHODIMP CGEKComponentSystemLight::Destroy(IGEKEntity *pEntity)
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

STDMETHODIMP_(void) CGEKComponentSystemLight::GetVisible(const frustum &kFrustum, concurrency::concurrent_unordered_set<IGEKEntity *> &aVisibleEntities)
{
    concurrency::parallel_for_each(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentLight>>::value_type &kPair) -> void
    {
        if (kPair.second->m_nRange > 0.0f)
        {
            IGEKComponent *pTransform = kPair.first->GetComponent(L"transform");
            if (pTransform != nullptr)
            {
                GEKVALUE kPosition;
                pTransform->GetProperty(L"position", kPosition);
                if (kFrustum.IsVisible(sphere(kPosition.GetFloat3(), kPair.second->m_nRange)))
                {
                    aVisibleEntities.insert(kPair.first);
                }
            }
        }
    });
}
