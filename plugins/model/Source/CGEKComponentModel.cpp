#include "CGEKComponentModel.h"
#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"

#define NUM_INSTANCES                   50

REGISTER_COMPONENT(model)
    REGISTER_COMPONENT_DEFAULT_VALUE(file, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(params, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(scale, float3(1.0f, 1.0f, 1.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(color, float4(1.0f, 1.0f, 1.0f, 1.0f))
    REGISTER_COMPONENT_SERIALIZE(model)
        REGISTER_COMPONENT_SERIALIZE_VALUE(file, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(scale, StrFromFloat3)
        REGISTER_COMPONENT_SERIALIZE_VALUE(color, StrFromFloat4)
    REGISTER_COMPONENT_DESERIALIZE(model)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(file, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(scale, StrToFloat3)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(color, StrToFloat4)
END_REGISTER_COMPONENT(model)

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

CGEKComponentSystemModel::MODEL *CGEKComponentSystemModel::GetModel(LPCWSTR pName, LPCWSTR pParams)
{
    REQUIRE_RETURN(m_pVideoSystem && m_pMaterialManager, nullptr);
    REQUIRE_RETURN(pName, nullptr);
    REQUIRE_RETURN(pParams, nullptr);

    MODEL *pModel = nullptr;
    auto pIterator = m_aModels.find(FormatString(L"%s|%s", pName, pParams));
    if (pIterator != m_aModels.end())
    {
        pModel = &(*pIterator).second;
    }
    else
    {
        std::vector<UINT8> aBuffer;
        HRESULT hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.gek", pName), aBuffer);
        if (SUCCEEDED(hRetVal))
        {
            UINT8 *pBuffer = aBuffer.data();
            UINT32 nGEKX = *((UINT32 *)pBuffer);
            pBuffer += sizeof(UINT32);

            UINT16 nType = *((UINT16 *)pBuffer);
            pBuffer += sizeof(UINT16);

            UINT16 nVersion = *((UINT16 *)pBuffer);
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
                    CStringW strMaterial(CA2W(strMaterialUTF8, CP_UTF8));

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
                    hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float3), nNumVertices, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &kModel.m_spPositionBuffer, pBuffer);
                    pBuffer += (sizeof(float3) * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float2), nNumVertices, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &kModel.m_spTexCoordBuffer, pBuffer);
                    pBuffer += (sizeof(float2) * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float3), nNumVertices, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &kModel.m_spNormalBuffer, pBuffer);
                    pBuffer += (sizeof(float3) * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    UINT32 nNumIndices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT16), nNumIndices, GEK3DVIDEO::BUFFER::INDEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &kModel.m_spIndexBuffer, pBuffer);
                    pBuffer += (sizeof(UINT16) * nNumIndices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    pModel = &(m_aModels[FormatString(L"%s|%s", pName, pParams)] = kModel);
                }
            }
        }
    }

    return pModel;
}

STDMETHODIMP CGEKComponentSystemModel::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    m_pVideoSystem = GetContext()->GetCachedClass<IGEK3DVideoSystem>(CLSID_GEKVideoSystem);
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
            hRetVal = spProgramManager->LoadProgram(L"model", &m_spVertexProgram);
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        IGEK3DVideoSystem *pVideoSystem = GetContext()->GetCachedClass<IGEK3DVideoSystem>(CLSID_GEKVideoSystem);
        if (pVideoSystem != nullptr)
        {
            hRetVal = pVideoSystem->CreateBuffer(sizeof(INSTANCE), NUM_INSTANCES, GEK3DVIDEO::BUFFER::DYNAMIC | GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER | GEK3DVIDEO::BUFFER::RESOURCE, &m_spInstanceBuffer);
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

STDMETHODIMP_(void) CGEKComponentSystemModel::OnRenderBegin(void)
{
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnCullScene(const frustum &nViewFrustum)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    m_aVisible.clear();
    m_pSceneManager->ListComponentsEntities({ GET_COMPONENT_ID(transform), GET_COMPONENT_ID(model) }, [&](const GEKENTITYID &nEntityID) -> void
    {
        auto &kModel = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(model)>(nEntityID, GET_COMPONENT_ID(model));

        MODEL *pModel = GetModel(kModel.file, kModel.params);
        if (pModel)
        {
            auto &kTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));

            float4x4 nMatrix;
            nMatrix   = kTransform.rotation;
            nMatrix.t = kTransform.position;

            aabb nAABB(pModel->m_nAABB);
            nAABB.minimum *= kModel.scale;
            nAABB.maximum *= kModel.scale;
            if (nViewFrustum.IsVisible(obb(nAABB, nMatrix)))
            {
                m_aVisible[pModel].emplace_back(nMatrix, kModel.scale, kModel.color);
            }
        }
    });
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnDrawScene(IGEK3DVideoContext *pContext, UINT32 nVertexAttributes)
{
    REQUIRE_VOID_RETURN(pContext);

    if (!(nVertexAttributes & GEK_VERTEX_POSITION) &&
        !(nVertexAttributes & GEK_VERTEX_TEXCOORD) &&
        !(nVertexAttributes & GEK_VERTEX_NORMAL))
    {
        return;
    }

    m_pProgramManager->EnableProgram(pContext, m_spVertexProgram);
    pContext->GetVertexSystem()->SetResource(0, m_spInstanceBuffer);
    pContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);
    for (auto kModel : m_aVisible)
    {
        if (nVertexAttributes & GEK_VERTEX_POSITION)
        {
            pContext->SetVertexBuffer(0, 0, kModel.first->m_spPositionBuffer);
        }

        if (nVertexAttributes & GEK_VERTEX_TEXCOORD)
        {
            pContext->SetVertexBuffer(1, 0, kModel.first->m_spTexCoordBuffer);
        }

        if (nVertexAttributes & GEK_VERTEX_NORMAL)
        {
            pContext->SetVertexBuffer(2, 0, kModel.first->m_spNormalBuffer);
        }

        pContext->SetIndexBuffer(0, kModel.first->m_spIndexBuffer);
        for (UINT32 nPass = 0; nPass < kModel.second.size(); nPass += NUM_INSTANCES)
        {
            UINT32 nNumInstances = min(NUM_INSTANCES, (kModel.second.size() - nPass));

            INSTANCE *pInstances = nullptr;
            if (SUCCEEDED(m_spInstanceBuffer->Map((LPVOID *)&pInstances)))
            {
                memcpy(pInstances, &kModel.second[nPass], (sizeof(INSTANCE) * nNumInstances));
                m_spInstanceBuffer->UnMap();

                for (auto &kMaterial : kModel.first->m_aMaterials)
                {
                    if (m_pMaterialManager->EnableMaterial(pContext, kMaterial.first))
                    {
                        pContext->DrawInstancedIndexedPrimitive(kMaterial.second.m_nNumIndices, nNumInstances, kMaterial.second.m_nFirstIndex, kMaterial.second.m_nFirstVertex, 0);
                    }
                }
            }
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnRenderEnd(void)
{
}
