﻿#include "CGEKComponentModel.h"
#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"
#include "GEKComponents.h"
#include <ppl.h>

#define NUM_INSTANCES                   500

REGISTER_COMPONENT(model)
    REGISTER_COMPONENT_DEFAULT_VALUE(source, L"")
    REGISTER_COMPONENT_SERIALIZE(model)
        REGISTER_COMPONENT_SERIALIZE_VALUE(source, )
    REGISTER_COMPONENT_DESERIALIZE(model)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(source, )
END_REGISTER_COMPONENT(model)

BEGIN_INTERFACE_LIST(CGEKComponentSystemModel)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemModel)

CGEKComponentSystemModel::CGEKComponentSystemModel(void)
    : m_pEngine(nullptr)
{
}

CGEKComponentSystemModel::~CGEKComponentSystemModel(void)
{
    CGEKObservable::RemoveObserver(m_pEngine->GetSceneManager(), GetClass<IGEKSceneObserver>());
    CGEKObservable::RemoveObserver(m_pEngine->GetRenderManager(), GetClass<IGEKRenderObserver>());
}

CGEKComponentSystemModel::MODEL *CGEKComponentSystemModel::GetModel(LPCWSTR pSource)
{
    REQUIRE_RETURN(pSource, nullptr);

    MODEL *pModel = nullptr;
    auto pIterator = m_aModels.find(pSource);
    if (pIterator != m_aModels.end())
    {
        if ((*pIterator).second.m_bLoaded)
        {
            pModel = &(*pIterator).second;
        }
    }
    else
    {
        MODEL &kModel = m_aModels[pSource];

        std::vector<UINT8> aBuffer;
        HRESULT hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.gek", pSource), aBuffer);
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
                    hRetVal = m_pEngine->GetMaterialManager()->LoadMaterial(strMaterial, &spMaterial);
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
                    hRetVal = m_pEngine->GetVideoSystem()->CreateBuffer(sizeof(Math::Float3), nNumVertices, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &kModel.m_spPositionBuffer, pBuffer);
                    pBuffer += (sizeof(Math::Float3) * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_pEngine->GetVideoSystem()->CreateBuffer(sizeof(Math::Float2), nNumVertices, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &kModel.m_spTexCoordBuffer, pBuffer);
                    pBuffer += (sizeof(Math::Float2) * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_pEngine->GetVideoSystem()->CreateBuffer(sizeof(Math::Float3), nNumVertices, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &kModel.m_spNormalBuffer, pBuffer);
                    pBuffer += (sizeof(Math::Float3) * nNumVertices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    UINT32 nNumIndices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    hRetVal = m_pEngine->GetVideoSystem()->CreateBuffer(sizeof(UINT16), nNumIndices, GEK3DVIDEO::BUFFER::INDEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &kModel.m_spIndexBuffer, pBuffer);
                    pBuffer += (sizeof(UINT16) * nNumIndices);
                }

                if (SUCCEEDED(hRetVal))
                {
                    kModel.m_bLoaded = true;
                    pModel = &kModel;
                }
            }
        }
    }

    return pModel;
}

STDMETHODIMP CGEKComponentSystemModel::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine->GetRenderManager(), GetClass<IGEKRenderObserver>());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(m_pEngine->GetSceneManager(), GetClass<IGEKSceneObserver>());
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pEngine->GetProgramManager()->LoadProgram(L"model", &m_spVertexProgram);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pEngine->GetVideoSystem()->CreateBuffer(sizeof(INSTANCE), NUM_INSTANCES, GEK3DVIDEO::BUFFER::DYNAMIC | GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER | GEK3DVIDEO::BUFFER::RESOURCE, &m_spInstanceBuffer);
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemModel::OnLoadEnd(HRESULT hRetVal)
{
    return S_OK;
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnFree(void)
{
    m_aModels.clear();
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnRenderBegin(const GEKENTITYID &nViewerID)
{
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnCullScene(const GEKENTITYID &nViewerID, const frustum &nViewFrustum)
{
    m_aVisible.clear();
    m_pEngine->GetSceneManager()->ListComponentsEntities({ GET_COMPONENT_ID(transform), GET_COMPONENT_ID(model) }, [&](const GEKENTITYID &nEntityID) -> void
    {
        auto &kModel = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(model)>(nEntityID, GET_COMPONENT_ID(model));
        MODEL *pModel = GetModel(kModel.source);
        if (pModel)
        {
            Math::Float3 nSize(1.0f, 1.0f, 1.0f);
            if (m_pEngine->GetSceneManager()->HasComponent(nEntityID, GET_COMPONENT_ID(size)))
            {
                nSize = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(size)>(nEntityID, GET_COMPONENT_ID(size)).value;
            }

            Math::Float4 nColor(1.0f, 1.0f, 1.0f, 1.0f);
            if (m_pEngine->GetSceneManager()->HasComponent(nEntityID, GET_COMPONENT_ID(color)))
            {
                nColor = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(color)>(nEntityID, GET_COMPONENT_ID(color)).value;
            }

            auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
            Math::Float4x4 nMatrix(kTransform.rotation, kTransform.position);

            aabb nAABB(pModel->m_nAABB);
            nAABB.minimum *= nSize;
            nAABB.maximum *= nSize;
            if (nViewFrustum.IsVisible(obb(nAABB, nMatrix)))
            {
                m_aVisible[pModel].emplace_back(nMatrix, nSize, nColor, nViewFrustum.origin.Distance(kTransform.position));
            }
        }
    });

    for (auto kModel : m_aVisible)
    {
        concurrency::parallel_sort(kModel.second.begin(), kModel.second.end(), [&](const INSTANCE &kInstanceA, const INSTANCE &kInstanceB) -> bool
        {
            return (kInstanceA.m_nDistance < kInstanceB.m_nDistance);
        });
    }
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnDrawScene(const GEKENTITYID &nViewerID, IGEK3DVideoContext *pContext, UINT32 nVertexAttributes)
{
    REQUIRE_VOID_RETURN(pContext);

    if (!(nVertexAttributes & GEK_VERTEX_POSITION) &&
        !(nVertexAttributes & GEK_VERTEX_TEXCOORD) &&
        !(nVertexAttributes & GEK_VERTEX_NORMAL))
    {
        return;
    }

    m_pEngine->GetProgramManager()->EnableProgram(pContext, m_spVertexProgram);
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
            INSTANCE *pInstances = nullptr;
            if (SUCCEEDED(m_spInstanceBuffer->Map((LPVOID *)&pInstances)))
            {
                UINT32 nNumInstances = std::min(NUM_INSTANCES, (kModel.second.size() - nPass));
                memcpy(pInstances, kModel.second.data(), (sizeof(INSTANCE) * nNumInstances));
                m_spInstanceBuffer->UnMap();

                for (auto &kMaterial : kModel.first->m_aMaterials)
                {
                    if (m_pEngine->GetMaterialManager()->EnableMaterial(pContext, kMaterial.first))
                    {
                        pContext->DrawInstancedIndexedPrimitive(kMaterial.second.m_nNumIndices, nNumInstances, kMaterial.second.m_nFirstIndex, kMaterial.second.m_nFirstVertex, 0);
                    }
                }
            }
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemModel::OnRenderEnd(const GEKENTITYID &nViewerID)
{
}
