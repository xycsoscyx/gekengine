#include "CGEKStaticModel.h"
#include <algorithm>

BEGIN_INTERFACE_LIST(CGEKStaticModel)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoSystemUser)
    INTERFACE_LIST_ENTRY_COM(IGEKProgramManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterialManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKStaticFactoryUser)
    INTERFACE_LIST_ENTRY_COM(IGEKResource)
    INTERFACE_LIST_ENTRY_COM(IGEKModel)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKStaticModel)

CGEKStaticModel::CGEKStaticModel(void)
{
}

CGEKStaticModel::~CGEKStaticModel(void)
{
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
            hRetVal = GetMaterialManager()->LoadMaterial(strMaterial, &spMaterial);
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
            hRetVal = GetVideoSystem()->CreateBuffer(sizeof(float3), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spPositionBuffer, pBuffer);
            pBuffer += (sizeof(float3) * nNumVertices);
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = GetVideoSystem()->CreateBuffer(sizeof(float2), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spTexCoordBuffer, pBuffer);
            pBuffer += (sizeof(float2) * nNumVertices);
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = GetVideoSystem()->CreateBuffer((sizeof(float3) * 3), nNumVertices, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spBasisBuffer, pBuffer);
            pBuffer += (sizeof(float3) * 3 * nNumVertices);
        }

        if (SUCCEEDED(hRetVal))
        {
            UINT32 nNumIndices = *((UINT32 *)pBuffer);
            pBuffer += sizeof(UINT32);

            hRetVal = GetVideoSystem()->CreateBuffer(sizeof(UINT16), nNumIndices, GEKVIDEO::BUFFER::INDEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spIndexBuffer, pBuffer);
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
        GetMaterialManager()->PrepareMaterial(kPair.first);
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

    GetProgramManager()->EnableProgram(GetStaticFactory()->GetVertexProgram());
    GetVideoSystem()->GetImmediateContext()->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);
    if (nVertexAttributes & GEK_VERTEX_POSITION)
    {
        GetVideoSystem()->GetImmediateContext()->SetVertexBuffer(0, 0, m_spPositionBuffer);
    }

    if (nVertexAttributes & GEK_VERTEX_TEXCOORD)
    {
        GetVideoSystem()->GetImmediateContext()->SetVertexBuffer(1, 0, m_spTexCoordBuffer);
    }

    if (nVertexAttributes & GEK_VERTEX_BASIS)
    { 
        GetVideoSystem()->GetImmediateContext()->SetVertexBuffer(2, 0, m_spBasisBuffer);
    }

    GetVideoSystem()->GetImmediateContext()->SetIndexBuffer(0, m_spIndexBuffer);
    GetVideoSystem()->GetImmediateContext()->GetVertexSystem()->SetResource(0, GetStaticFactory()->GetInstanceBuffer());
    for (UINT32 nPass = 0; nPass < aInstances.size(); nPass += GetStaticFactory()->GetNumInstances())
    {
        UINT32 nNumInstances = min(GetStaticFactory()->GetNumInstances(), (aInstances.size() - nPass));

        IGEKModel::INSTANCE *pInstances = nullptr;
        if (SUCCEEDED(GetStaticFactory()->GetInstanceBuffer()->Map((LPVOID *)&pInstances)))
        {
            memcpy(pInstances, &aInstances[nPass], (sizeof(IGEKModel::INSTANCE) * nNumInstances));
            GetStaticFactory()->GetInstanceBuffer()->UnMap();
            for (auto &kPair : m_aMaterials)
            {
                if (GetMaterialManager()->EnableMaterial(kPair.first))
                {
                    GetVideoSystem()->GetImmediateContext()->DrawInstancedIndexedPrimitive(kPair.second.m_nNumIndices, nNumInstances, kPair.second.m_nFirstIndex, kPair.second.m_nFirstVertex, 0);
                }
            }
        }
    }
}
