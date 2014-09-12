#include "CGEKComponentSystemModel.h"

#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"

#define NUM_INSTANCES                   50

BEGIN_INTERFACE_LIST(CGEKComponentSystemModel)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemModel)

CGEKComponentSystemModel::CGEKComponentSystemModel(void)
    : m_pRenderManager(nullptr)
    , m_pSceneManager(nullptr)
    , m_pVideoSystem(nullptr)
    , m_pMaterialManager(nullptr)
    , m_pProgramManager(nullptr)
{
}

CGEKComponentSystemModel::~CGEKComponentSystemModel(void)
{
}

HRESULT CGEKComponentSystemModel::GetModel(LPCWSTR pName, LPCWSTR pParams, MODEL **ppModel)
{
    concurrency::critical_section::scoped_lock kLock(m_kCritical);
    REQUIRE_RETURN(m_pVideoSystem && m_pMaterialManager, E_FAIL);
    REQUIRE_RETURN(ppModel, E_INVALIDARG);
    REQUIRE_RETURN(pName, E_INVALIDARG);
    REQUIRE_RETURN(pParams, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aModels.find(FormatString(L"%s|%s", pName, pParams));
    if (pIterator != m_aModels.end())
    {
        (*ppModel) = &(*pIterator).second;
        hRetVal = S_OK;
    }
    else
    {
        GEKFUNCTION(L"Name(%s), Params(%s)", pName, pParams);

        std::vector<UINT8> aBuffer;
        hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.model.gek", pName), aBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to Load Model File failed: 0x%08X", hRetVal);
        if (SUCCEEDED(hRetVal))
        {
            UINT8 *pBuffer = &aBuffer[0];
            UINT32 nGEKX = *((UINT32 *)pBuffer);
            GEKRESULT(nGEKX == *(UINT32 *)"GEKX", L"Invalid Magic Header: %d", nGEKX);
            pBuffer += sizeof(UINT32);

            UINT16 nType = *((UINT16 *)pBuffer);
            GEKRESULT(nType == 0, L"Invalid Header Type: %d", nType);
            pBuffer += sizeof(UINT16);

            UINT16 nVersion = *((UINT16 *)pBuffer);
            GEKRESULT(nVersion == 2, L"Invalid Header Version: %d", nVersion);
            pBuffer += sizeof(UINT16);

            HRESULT hRetVal = E_INVALIDARG;
            if (nGEKX == *(UINT32 *)"GEKX" && nType == 0 && nVersion == 2)
            {
                MODEL kModel;
                kModel.m_nAABB = *(aabb *)pBuffer;
                pBuffer += sizeof(aabb);

                UINT32 nNumMaterials = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);

                for (UINT32 nMaterial = 0; nMaterial < nNumMaterials; ++nMaterial)
                {
                    CStringA strMaterialUTF8 = pBuffer;
                    pBuffer += (strMaterialUTF8.GetLength() + 1);
                    CStringW strMaterial = CA2W(strMaterialUTF8, CP_UTF8);

                    CComPtr<IUnknown> spMaterial;
                    hRetVal = m_pMaterialManager->LoadMaterial(strMaterial, &spMaterial);
                    if (SUCCEEDED(hRetVal))
                    {
                        MATERIAL kMaterial;
                        kMaterial.m_nFirstVertex = *((UINT32 *)pBuffer);
                        pBuffer += sizeof(UINT32);

                        kMaterial.m_nFirstIndex = *((UINT32 *)pBuffer);
                        pBuffer += sizeof(UINT32);

                        kMaterial.m_nNumIndices = *((UINT32 *)pBuffer);
                        pBuffer += sizeof(UINT32);

                        kModel.m_aMaterials.insert(std::make_pair(spMaterial, kMaterial));
                    }
                    else
                    {
                        break;
                    }
                }

                UINT32 nNumVertices = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);

                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float3), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &kModel.m_spPositionBuffer, pBuffer);
                    pBuffer += (sizeof(float3) * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float2), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &kModel.m_spTexCoordBuffer, pBuffer);
                    pBuffer += (sizeof(float2) * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_pVideoSystem->CreateBuffer((sizeof(float3) * 3), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &kModel.m_spBasisBuffer, pBuffer);
                    pBuffer += (sizeof(float3) * 3 * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    UINT32 nNumIndices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT16), nNumIndices, GEKVIDEO::BUFFER::INDEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &kModel.m_spIndexBuffer, pBuffer);
                    pBuffer += (sizeof(UINT16) * nNumIndices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    m_aModels[FormatString(L"%s|%s", pName, pParams)] = kModel;
                    (*ppModel) = &m_aModels[FormatString(L"%s|%s", pName, pParams)];
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemModel::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    m_pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
    m_pRenderManager = GetContext()->GetCachedClass<IGEKRenderManager>(CLSID_GEKRenderSystem);
    m_pProgramManager = GetContext()->GetCachedClass<IGEKProgramManager>(CLSID_GEKRenderSystem);
    m_pMaterialManager = GetContext()->GetCachedClass<IGEKMaterialManager>(CLSID_GEKRenderSystem);
    if (m_pRenderManager && m_pSceneManager && m_pVideoSystem && m_pMaterialManager && m_pProgramManager)
    {
        hRetVal = CGEKObservable::AddObserver(m_pRenderManager, (IGEKRenderObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        CComQIPtr<IGEKProgramManager> spProgramManager(m_pRenderManager);
        if (spProgramManager != nullptr)
        {
            hRetVal = spProgramManager->LoadProgram(L"staticmodel", &m_spVertexProgram);
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        IGEKVideoSystem *pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
        if (pVideoSystem != nullptr)
        {
            hRetVal = pVideoSystem->CreateBuffer(sizeof(IGEKModel::INSTANCE), NUM_INSTANCES, GEKVIDEO::BUFFER::DYNAMIC | GEKVIDEO::BUFFER::STRUCTURED_BUFFER | GEKVIDEO::BUFFER::RESOURCE, &m_spInstanceBuffer);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemModel::Destroy(void)
{
    CGEKObservable::RemoveObserver(m_pRenderManager, (IGEKRenderObserver *)GetUnknown());
    CGEKObservable::RemoveObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
}

STDMETHODIMP CGEKComponentSystemModel::OnLoadEnd(HRESULT hRetVal)
{
    return S_OK;
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnFree(void)
{
    m_aModels.clear();
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnPreRender(void)
{
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnCullScene(void)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    m_pSceneManager->ListComponentsEntities({ L"transform", L"model" }, [&](const GEKENTITYID &nEntityID)->void
    {
        GEKVALUE kSource;
        GEKVALUE kParams;
        m_pSceneManager->GetProperty(nEntityID, L"model", L"source", kSource);
        m_pSceneManager->GetProperty(nEntityID, L"model", L"params", kParams);

        MODEL *pModel = nullptr;
        if (SUCCEEDED(GetModel(kSource.GetRawString(), kParams.GetRawString(), &pModel)))
        {
            INSTANCE kInstance;

            GEKVALUE kScale;
            m_pSceneManager->GetProperty(nEntityID, L"model", L"scale", kScale);
            kInstance.m_nScale = kScale.GetFloat3();

            GEKVALUE kPosition;
            GEKVALUE kRotation;
            m_pSceneManager->GetProperty(nEntityID, L"transform", L"position", kPosition);
            m_pSceneManager->GetProperty(nEntityID, L"transform", L"rotation", kRotation);
            kInstance.m_nMatrix = kRotation.GetQuaternion();
            kInstance.m_nMatrix.t = kPosition.GetFloat3();

            m_aVisible[pModel].push_back(kInstance);
        }
    });
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnDrawScene(UINT32 nVertexAttributes)
{
    if (!(nVertexAttributes & GEK_VERTEX_POSITION) &&
        !(nVertexAttributes & GEK_VERTEX_TEXCOORD) &&
        !(nVertexAttributes & GEK_VERTEX_BASIS))
    {
        return;
    }

    m_pProgramManager->EnableProgram(m_spVertexProgram);
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetResource(0, m_spInstanceBuffer);
    m_pVideoSystem->GetImmediateContext()->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);
    for (auto kModel : m_aVisible)
    {
        if (nVertexAttributes & GEK_VERTEX_POSITION)
        {
            m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(0, 0, kModel.first->m_spPositionBuffer);
        }

        if (nVertexAttributes & GEK_VERTEX_TEXCOORD)
        {
            m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(1, 0, kModel.first->m_spTexCoordBuffer);
        }

        if (nVertexAttributes & GEK_VERTEX_BASIS)
        {
            m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(2, 0, kModel.first->m_spBasisBuffer);
        }

        m_pVideoSystem->GetImmediateContext()->SetIndexBuffer(0, kModel.first->m_spIndexBuffer);
        for (UINT32 nPass = 0; nPass < kModel.second.size(); nPass += NUM_INSTANCES)
        {
            UINT32 nNumInstances = min(NUM_INSTANCES, (kModel.second.size() - nPass));

            IGEKModel::INSTANCE *pInstances = nullptr;
            if (SUCCEEDED(m_spInstanceBuffer->Map((LPVOID *)&pInstances)))
            {
                memcpy(pInstances, &kModel.second[nPass], (sizeof(IGEKModel::INSTANCE) * nNumInstances));
                m_spInstanceBuffer->UnMap();

                for (auto &kMaterial : kModel.first->m_aMaterials)
                {
                    if (m_pMaterialManager->EnableMaterial(kMaterial.first))
                    {
                        m_pVideoSystem->GetImmediateContext()->DrawInstancedIndexedPrimitive(kMaterial.second.m_nNumIndices, nNumInstances, kMaterial.second.m_nFirstIndex, kMaterial.second.m_nFirstVertex, 0);
                    }
                }
            }
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnPostRender(void)
{
    m_aVisible.clear();
}
