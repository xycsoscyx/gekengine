#include "CGEKStaticModel.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKModels.h"

BEGIN_INTERFACE_LIST(CGEKStaticModel)
    INTERFACE_LIST_ENTRY_COM(IGEKResource)
    INTERFACE_LIST_ENTRY_COM(IGEKModel)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKStaticModel)

CGEKStaticModel::CGEKStaticModel(void)
    : m_pVideoSystem(nullptr)
    , m_pMaterialManager(nullptr)
    , m_pProgramManager(nullptr)
    , m_pStaticFactory(nullptr)
{
}

CGEKStaticModel::~CGEKStaticModel(void)
{
}

STDMETHODIMP CGEKStaticModel::Initialize(void)
{
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    m_pMaterialManager = GetContext()->GetCachedClass<IGEKMaterialManager>(CLSID_GEKRenderManager);
    m_pProgramManager = GetContext()->GetCachedClass<IGEKProgramManager>(CLSID_GEKRenderManager);
    m_pStaticFactory = GetContext()->GetCachedClass<IGEKStaticFactory>(CLSID_GEKFactory);
    return (m_pVideoSystem &&
            m_pMaterialManager &&
            m_pProgramManager &&
            m_pStaticFactory ? S_OK : E_FAIL);
}

STDMETHODIMP CGEKStaticModel::Load(const UINT8 *pBuffer, LPCWSTR pParams)
{
    REQUIRE_RETURN(pBuffer, E_INVALIDARG);

    UINT32 nGEKX = *((UINT32 *)pBuffer);
    pBuffer += sizeof(UINT32);

    UINT16 nType = *((UINT16 *)pBuffer);
    pBuffer += sizeof(UINT16);

    UINT16 nVersion = *((UINT16 *)pBuffer);
    pBuffer += sizeof(UINT16);

    HRESULT hRetVal = E_INVALIDARG;
    if (nGEKX == *(UINT32 *)"GEKX" && nType == 0 && nVersion == 2)
    {
        m_nAABB = *(aabb *)pBuffer;
        pBuffer += sizeof(aabb);

        UINT32 nNumMaterials = *((UINT32 *)pBuffer);
        pBuffer += sizeof(UINT32);

        for (UINT32 nMaterial = 0; nMaterial < nNumMaterials; nMaterial++)
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

                m_aMaterials.insert(std::make_pair(spMaterial, kMaterial));
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
            hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float3), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spPositionBuffer, pBuffer);
            pBuffer += (sizeof(float3) * nNumVertices);
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float2), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spTexCoordBuffer, pBuffer);
            pBuffer += (sizeof(float2) * nNumVertices);
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = m_pVideoSystem->CreateBuffer((sizeof(float3) * 3), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spBasisBuffer, pBuffer);
            pBuffer += (sizeof(float3) * 3 * nNumVertices);
        }

        if (SUCCEEDED(hRetVal))
        {
            UINT32 nNumIndices = *((UINT32 *)pBuffer);
            pBuffer += sizeof(UINT32);

            hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT16), nNumIndices, GEKVIDEO::BUFFER::INDEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spIndexBuffer, pBuffer);
            pBuffer += (sizeof(UINT16) * nNumIndices);
        }
    }

    return hRetVal;
}

STDMETHODIMP_(aabb) CGEKStaticModel::GetAABB(void)
{
    return m_nAABB;
}

STDMETHODIMP_(void) CGEKStaticModel::Prepare(void)
{
    for (auto &kPair : m_aMaterials)
    {
        m_pMaterialManager->PrepareMaterial(kPair.first);
    }
}

STDMETHODIMP_(void) CGEKStaticModel::Draw(UINT32 nVertexAttributes, const std::vector<IGEKModel::INSTANCE> &aInstances)
{
    if (!(nVertexAttributes & GEK_VERTEX_POSITION) &&
        !(nVertexAttributes & GEK_VERTEX_TEXCOORD) &&
        !(nVertexAttributes & GEK_VERTEX_BASIS))
    {
        return;
    }

    m_pProgramManager->EnableProgram(m_pStaticFactory->GetVertexProgram());
    m_pVideoSystem->GetImmediateContext()->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);
    if (nVertexAttributes & GEK_VERTEX_POSITION)
    {
        m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(0, 0, m_spPositionBuffer);
    }

    if (nVertexAttributes & GEK_VERTEX_TEXCOORD)
    {
        m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(1, 0, m_spTexCoordBuffer);
    }

    if (nVertexAttributes & GEK_VERTEX_BASIS)
    { 
        m_pVideoSystem->GetImmediateContext()->SetVertexBuffer(2, 0, m_spBasisBuffer);
    }

    m_pVideoSystem->GetImmediateContext()->SetIndexBuffer(0, m_spIndexBuffer);
    m_pVideoSystem->GetImmediateContext()->GetVertexSystem()->SetResource(0, m_pStaticFactory->GetInstanceBuffer());
    for (UINT32 nPass = 0; nPass < aInstances.size(); nPass += m_pStaticFactory->GetNumInstances())
    {
        UINT32 nNumInstances = min(m_pStaticFactory->GetNumInstances(), (aInstances.size() - nPass));

        IGEKModel::INSTANCE *pInstances = nullptr;
        if (SUCCEEDED(m_pStaticFactory->GetInstanceBuffer()->Map((LPVOID *)&pInstances)))
        {
            memcpy(pInstances, &aInstances[nPass], (sizeof(IGEKModel::INSTANCE) * nNumInstances));
            m_pStaticFactory->GetInstanceBuffer()->UnMap();
            for (auto &kPair : m_aMaterials)
            {
                if (m_pMaterialManager->EnableMaterial(kPair.first))
                {
                    m_pVideoSystem->GetImmediateContext()->DrawInstancedIndexedPrimitive(kPair.second.m_nNumIndices, nNumInstances, kPair.second.m_nFirstIndex, kPair.second.m_nFirstVertex, 0);
                }
            }
        }
    }
}
