#include "CGEKComponentModel.h"
#include "GEKAPI.h"
#include <algorithm>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKComponentModel)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentModel::CGEKComponentModel(IGEKContext *pContext, IGEKEntity *pEntity)
    : CGEKUnknown(pContext)
    , CGEKComponent(pEntity)
    , m_nScale(1.0f)
{
}

CGEKComponentModel::~CGEKComponentModel(void)
{
}

STDMETHODIMP_(void) CGEKComponentModel::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"source", m_strSource.GetString());
    OnProperty(L"params", m_strParams.GetString());
    OnProperty(L"model", (IUnknown *)m_spModel);
    OnProperty(L"scale", m_nScale);
}

static GEKHASH gs_nSource(L"source");
static GEKHASH gs_nParams(L"params");
static GEKHASH gs_nModel(L"model");
static GEKHASH gs_nScale(L"scale");
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
    else if (nHash == gs_nScale)
    {
        kValue = m_nScale;
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
    else if (nHash == gs_nScale)
    {
        m_nScale = kValue.GetFloat3();
        return true;
    }

    return false;
}

STDMETHODIMP CGEKComponentModel::OnEntityCreated(void)
{
    HRESULT hRetVal = S_OK;
    if (!m_strSource.IsEmpty())
    {
        IGEKModelManager *pModelManager = GetContext()->GetCachedClass<IGEKModelManager>(CLSID_GEKModelManager);
        if (pModelManager != nullptr)
        {
            hRetVal = pModelManager->LoadModel(m_strSource, m_strParams, &m_spModel);
        }
    }

    return hRetVal;
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemModel)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemModel)

CGEKComponentSystemModel::CGEKComponentSystemModel(void)
{
}

CGEKComponentSystemModel::~CGEKComponentSystemModel(void)
{
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnFree(void)
{
    m_aComponents.clear();
}

STDMETHODIMP CGEKComponentSystemModel::Initialize(void)
{
    return GetContext()->AddCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
};

STDMETHODIMP_(void) CGEKComponentSystemModel::Destroy(void)
{
    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
}

STDMETHODIMP_(LPCWSTR) CGEKComponentSystemModel::GetType(void) const
{
    return L"model";
}

STDMETHODIMP CGEKComponentSystemModel::Create(const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_OUTOFMEMORY;
    CComPtr<CGEKComponentModel> spComponent(new CGEKComponentModel(GetContext(), pEntity));
    GEKRESULT(spComponent, L"Unable to allocate new model component instance");
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

STDMETHODIMP CGEKComponentSystemModel::Destroy(IGEKEntity *pEntity)
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


STDMETHODIMP_(void) CGEKComponentSystemModel::GetVisible(const frustum &kFrustum, concurrency::concurrent_unordered_set<IGEKEntity *> &aVisibleEntities)
{
    concurrency::parallel_for_each(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentModel>>::value_type &kPair) -> void
    {
        CComQIPtr<IGEKModel> spModel(kPair.second->m_spModel);
        if (spModel)
        {
            IGEKComponent *pTransform = kPair.first->GetComponent(L"transform");
            if (pTransform != nullptr)
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