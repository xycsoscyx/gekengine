#include "CGEKStaticCollision.h"
#include <algorithm>

BEGIN_INTERFACE_LIST(CGEKStaticCollision)
    INTERFACE_LIST_ENTRY_COM(IGEKStaticData)
    INTERFACE_LIST_ENTRY_COM(IGEKCollision)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKStaticCollision)

CGEKStaticCollision::CGEKStaticCollision(void)
{
}

CGEKStaticCollision::~CGEKStaticCollision(void)
{
}

STDMETHODIMP CGEKStaticCollision::Load(const UINT8 *pBuffer, UINT32 nBufferSize)
{
    REQUIRE_RETURN(pBuffer, E_INVALIDARG);

    UINT32 nGEKX = *((UINT32 *)pBuffer);
    pBuffer += sizeof(UINT32);

    UINT16 nType = *((UINT16 *)pBuffer);
    pBuffer += sizeof(UINT16);

    UINT16 nVersion = *((UINT16 *)pBuffer);
    pBuffer += sizeof(UINT16);

    HRESULT hRetVal = E_INVALIDARG;
    if (nGEKX == *(UINT32 *)"GEKX" && nType == 1 && nVersion == 2)
    {
        m_nAABB = *(aabb *)pBuffer;
        pBuffer += sizeof(aabb);

        UINT32 nNumVertices = *((UINT32 *)pBuffer);
        pBuffer += sizeof(UINT32);
        float3 *pVertices = (float3 *)pBuffer;
        pBuffer += (sizeof(float3) * nNumVertices);
        if (nNumVertices > 0)
        {
            m_aVertices.assign(pVertices, (float3 *)pBuffer);

            UINT32 nNumIndices = *((UINT32 *)pBuffer);
            pBuffer += sizeof(UINT32);
            UINT16 *pIndices = (UINT16 *)pBuffer;
            pBuffer += (sizeof(UINT16) * nNumIndices);
            if (nNumVertices > 0)
            {
                m_aIndices.assign(pIndices, (UINT16 *)pBuffer);
                hRetVal = S_OK;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(UINT32) CGEKStaticCollision::GetNumVertices(void)
{
    return m_aVertices.size();
}

STDMETHODIMP_(float3 *) CGEKStaticCollision::GetVertices(void)
{
    return &m_aVertices[0];
}

STDMETHODIMP_(UINT32) CGEKStaticCollision::GetNumIndices(void)
{
    return m_aIndices.size();
}

STDMETHODIMP_(UINT16 *) CGEKStaticCollision::GetIndices(void)
{
    return &m_aIndices[0];
}

