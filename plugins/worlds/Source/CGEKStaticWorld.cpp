#include "CGEKStaticWorld.h"
#include <algorithm>

BEGIN_INTERFACE_LIST(CGEKStaticWorld)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoSystemUser)
    INTERFACE_LIST_ENTRY_COM(IGEKProgramManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterialManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKWorld)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKStaticWorld)

CGEKStaticWorld::CGEKStaticWorld(void)
    : m_nArea(0)
{
}

CGEKStaticWorld::~CGEKStaticWorld(void)
{
}

STDMETHODIMP CGEKStaticWorld::Load(const UINT8 *pBuffer, std::function<HRESULT(float3 *, IUnknown *)> OnStaticFace)
{
    HRESULT hRetVal = GetProgramManager()->LoadProgram(L"staticworld", &m_spVertexProgram);
    if (SUCCEEDED(hRetVal))
    {
        UINT32 nGEKX = *((UINT32 *)pBuffer);
        pBuffer += sizeof(UINT32);

        UINT16 nType = *((UINT16 *)pBuffer);
        pBuffer += sizeof(UINT16);

        UINT16 nVersion = *((UINT16 *)pBuffer);
        pBuffer += sizeof(UINT16);

        hRetVal = E_INVALIDARG;
        if (nGEKX == *(UINT32 *)"GEKX" && nType == 10 && nVersion == 1)
        {
            m_nAABB = *(aabb *)pBuffer;
            pBuffer += sizeof(aabb);

            UINT32 nNumAreas = *((UINT32 *)pBuffer);
            pBuffer += sizeof(UINT32);

            m_aAreas.resize(nNumAreas);
            for (UINT32 nArea = 0; nArea < nNumAreas; nArea++)
            {
                AREA &kArea = m_aAreas[nArea];
                kArea.m_bRenderData = false;

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

                        kArea.m_aMaterials.insert(std::make_pair(spMaterial, kMaterial));
                    }
                    else
                    {
                        break;
                    }
                }

                if (SUCCEEDED(hRetVal))
                {
                    UINT32 nNumVertices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    float3 *pPositionBuffer = (float3 *)pBuffer;
                    pBuffer += (sizeof(float3) * nNumVertices);
                    float3 *pTexCoordBuffer = (float3 *)pBuffer;
                    pBuffer += (sizeof(float2) * nNumVertices);
                    float3 *pBasisBuffer = (float3 *)pBuffer;
                    pBuffer += (sizeof(float3) * 3 * nNumVertices);

                    UINT32 nNumIndices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    UINT16 *pIndices = (UINT16 *)pBuffer;
                    pBuffer += (sizeof(UINT16) * nNumIndices);

                    if (nNumVertices > 0 && nNumIndices > 0)
                    {
                        kArea.m_bRenderData = true;
                        hRetVal = GetVideoSystem()->CreateVertexBuffer(pPositionBuffer, sizeof(float3), nNumVertices, &kArea.m_spPositionBuffer);
                        if (SUCCEEDED(hRetVal))
                        {
                            hRetVal = GetVideoSystem()->CreateVertexBuffer(pTexCoordBuffer, sizeof(float2), nNumVertices, &kArea.m_spTexCoordBuffer);
                        }

                        if (SUCCEEDED(hRetVal))
                        {
                            hRetVal = GetVideoSystem()->CreateVertexBuffer(pBasisBuffer, (sizeof(float3)* 3), nNumVertices, &kArea.m_spBasisBuffer);
                        }

                        if (SUCCEEDED(hRetVal))
                        {
                            hRetVal = GetVideoSystem()->CreateIndexBuffer(pIndices, GEKVIDEO::DATA::UINT16, nNumIndices, &kArea.m_spIndexBuffer);
                        }

                        if (SUCCEEDED(hRetVal))
                        {
                            for (auto &kPair : kArea.m_aMaterials)
                            {
                                for (UINT32 nFace = 0; nFace < kPair.second.m_nNumIndices; nFace += 3)
                                {
                                    float3 aFace[3] =
                                    {
                                        pPositionBuffer[kPair.second.m_nFirstVertex + pIndices[kPair.second.m_nFirstIndex + nFace + 0]],
                                        pPositionBuffer[kPair.second.m_nFirstVertex + pIndices[kPair.second.m_nFirstIndex + nFace + 1]],
                                        pPositionBuffer[kPair.second.m_nFirstVertex + pIndices[kPair.second.m_nFirstIndex + nFace + 2]],
                                    };

                                    OnStaticFace(aFace, kPair.first);
                                }
                            }
                        }
                    }
                }

                if (FAILED(hRetVal))
                {
                    break;
                }
            }

            if (SUCCEEDED(hRetVal))
            {
                // Nodes
                UINT32 nNumNodes = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);
                if (nNumNodes > 0)
                {
                    m_aNodes.resize(nNumNodes);
                    memcpy(&m_aNodes[0], pBuffer, (sizeof(NODE)* nNumNodes));
                    pBuffer += (sizeof(NODE)* nNumNodes);
                }

                // Portal Edges
                UINT32 nNumPortalEdges = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);
                if (nNumPortalEdges > 0)
                {
                    m_aPortalEdges.resize(nNumPortalEdges);
                    memcpy(&m_aPortalEdges[0], pBuffer, (sizeof(float3)* nNumPortalEdges));
                    pBuffer += (sizeof(float3)* nNumPortalEdges);
                }

                // Portals
                UINT32 nNumPortals = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);
                if (nNumPortals > 0)
                {
                    m_aPortals.resize(nNumPortals);
                    memcpy(&m_aPortals[0], pBuffer, (sizeof(PORTAL)* nNumPortals));
                    pBuffer += (sizeof(PORTAL)* nNumPortals);
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(aabb) CGEKStaticWorld::GetAABB(void)
{
    return m_nAABB;
}

STDMETHODIMP_(void) CGEKStaticWorld::Prepare(const frustum &nFrustum)
{
    m_nArea = GetArea(nFrustum.origin);
    for (auto &kArea : m_aAreas)
    {
        if (kArea.m_bRenderData)
        {
            for (auto &kPair : kArea.m_aMaterials)
            {
                GetMaterialManager()->PrepareMaterial(kPair.first);
            }
        }
    }
}

int CGEKStaticWorld::GetArea(const float3 &nPoint)
{
    INT32 nNode = 0;
    if (m_aNodes.size() > 0)
    {
        do
        {
            if (m_aNodes[nNode].Distance(nPoint) >= 0.0f)
            {
                nNode = m_aNodes[nNode].m_nPositiveChild;
            }
            else
            {
                nNode = m_aNodes[nNode].m_nNegativeChild;
            }
        } while (nNode > 0);
    }
    
    return (nNode == 0 ? 0 : (-nNode - 1));
}

STDMETHODIMP_(bool) CGEKStaticWorld::IsVisible(const aabb &nBox)
{
    return true;
    return (m_nArea < 0 ? false : (GetArea(nBox.GetCenter()) == m_nArea));
}

STDMETHODIMP_(bool) CGEKStaticWorld::IsVisible(const obb &nBox)
{
    return true;
    return (m_nArea < 0 ? false : (GetArea(nBox.position) == m_nArea));
}

STDMETHODIMP_(bool) CGEKStaticWorld::IsVisible(const sphere &nSphere)
{
    return true;
    return (m_nArea < 0 ? false : (GetArea(nSphere.position) == m_nArea));
}

STDMETHODIMP_(void) CGEKStaticWorld::Draw(UINT32 nVertexAttributes)
{
    if (!(nVertexAttributes & GEK_VERTEX_POSITION) &&
        !(nVertexAttributes & GEK_VERTEX_TEXCOORD) &&
        !(nVertexAttributes & GEK_VERTEX_BASIS))
    {
        return;
    }

    GetProgramManager()->EnableProgram(m_spVertexProgram);
    GetVideoSystem()->GetDefaultContext()->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);
    for (auto &kArea : m_aAreas)
    {
        if (kArea.m_bRenderData)
        {
            if (nVertexAttributes & GEK_VERTEX_POSITION)
            {
                GetVideoSystem()->GetDefaultContext()->SetVertexBuffer(0, 0, kArea.m_spPositionBuffer);
            }

            if (nVertexAttributes & GEK_VERTEX_TEXCOORD)
            {
                GetVideoSystem()->GetDefaultContext()->SetVertexBuffer(1, 0, kArea.m_spTexCoordBuffer);
            }

            if (nVertexAttributes & GEK_VERTEX_BASIS)
            {
                GetVideoSystem()->GetDefaultContext()->SetVertexBuffer(2, 0, kArea.m_spBasisBuffer);
            }

            GetVideoSystem()->GetDefaultContext()->SetIndexBuffer(0, kArea.m_spIndexBuffer);
            for (auto &kPair : kArea.m_aMaterials)
            {
                if (GetMaterialManager()->EnableMaterial(kPair.first))
                {
                    GetVideoSystem()->GetDefaultContext()->DrawIndexedPrimitive(kPair.second.m_nNumIndices, kPair.second.m_nFirstIndex, kPair.second.m_nFirstVertex);
                }
            }
        }
    }
}
