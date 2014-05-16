#include "CGEKComponentModel.h"
#include "GEKAPI.h"
#include <algorithm>
#include <ppl.h>

BEGIN_INTERFACE_LIST(CGEKComponentModel)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
    INTERFACE_LIST_ENTRY_COM(IGEKModelManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKViewManagerUser)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentModel::CGEKComponentModel(IGEKEntity *pEntity)
    : CGEKComponent(pEntity)
{
}

CGEKComponentModel::~CGEKComponentModel(void)
{
}

STDMETHODIMP_(LPCWSTR) CGEKComponentModel::GetType(void) const
{
    return L"model";
}

STDMETHODIMP_(void) CGEKComponentModel::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"source", m_strSource.GetString());
    OnProperty(L"params", m_strParams.GetString());
    OnProperty(L"model", (IUnknown *)m_spModel);
}

static GEKHASH gs_nSource(L"source");
static GEKHASH gs_nParams(L"params");
static GEKHASH gs_nModel(L"model");
STDMETHODIMP_(bool) CGEKComponentModel::GetProperty(LPCWSTR pName, GEKVALUE &kValue) const
{
    GEKHASH nHash(pName);
    if (nHash == gs_nSource)
    {
        kValue = m_strSource.GetString();
        return true;
    }
    else if (nHash == gs_nParams)
    {
        kValue = m_strParams.GetString();
        return true;
    }
    else if (nHash == gs_nModel)
    {
        kValue = (IUnknown *)m_spModel;
        return true;
    }

    return false;
}

STDMETHODIMP_(bool) CGEKComponentModel::SetProperty(LPCWSTR pName, const GEKVALUE &kValue)
{
    GEKHASH nHash(pName);
    if (nHash == gs_nSource)
    {
        m_strSource = kValue.GetString();
        return true;
    }
    else if (nHash == gs_nParams)
    {
        m_strParams = kValue.GetString();
        return true;
    }
    else if (nHash == gs_nModel)
    {
        m_spModel = kValue.GetObject();
        return true;
    }

    return false;
}

STDMETHODIMP CGEKComponentModel::OnEntityCreated(void)
{
    HRESULT hRetVal = S_OK;
    if (!m_strSource.IsEmpty())
    {
        hRetVal = GetModelManager()->LoadModel(m_strSource, m_strParams, &m_spModel);
    }

    return hRetVal;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemModel)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKViewManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemModel)

CGEKComponentSystemModel::CGEKComponentSystemModel(void)
{
}

CGEKComponentSystemModel::~CGEKComponentSystemModel(void)
{
}

STDMETHODIMP_(void) CGEKComponentSystemModel::Clear(void)
{
    m_aComponents.clear();
}

STDMETHODIMP CGEKComponentSystemModel::Create(const CLibXMLNode &kEntityNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_FAIL;
    if (kEntityNode.HasAttribute(L"type") && kEntityNode.GetAttribute(L"type").CompareNoCase(L"model") == 0)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKComponentModel> spComponent(new CGEKComponentModel(pEntity));
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
                kEntityNode.ListAttributes([&spComponent] (LPCWSTR pName, LPCWSTR pValue) -> void
                {
                    spComponent->SetProperty(pName, pValue);
                } );

                m_aComponents[pEntity] = spComponent;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemModel::Destroy(IGEKEntity *pEntity)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = std::find_if(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentModel>>::value_type &kPair) -> bool
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


STDMETHODIMP_(void) CGEKComponentSystemModel::GetVisible(const frustum &kFrustum, concurrency::concurrent_unordered_set<IGEKEntity *> &aVisibleEntities)
{
    concurrency::parallel_for_each(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentModel>>::value_type &kPair) -> void
    {
        CComQIPtr<IGEKModel> spModel(kPair.second->m_spModel);
        if (spModel)
        {
            IGEKComponent *pTransform = kPair.first->GetComponent(L"transform");
            if (pTransform)
            {
                GEKVALUE kPosition;
                GEKVALUE kRotation;
                pTransform->GetProperty(L"position", kPosition);
                pTransform->GetProperty(L"rotation", kRotation);

                IGEKModel::INSTANCE kInstance;
                kInstance.m_nMatrix = kRotation.GetQuaternion();
                kInstance.m_nMatrix.t = kPosition.GetFloat3();
                if (kFrustum.IsVisible(obb(spModel->GetAABB(), kInstance.m_nMatrix)))
                {
                    aVisibleEntities.insert(kPair.first);
                }
            }
        }
    });
}