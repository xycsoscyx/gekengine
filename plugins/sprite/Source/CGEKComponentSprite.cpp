#include "CGEKComponentSprite.h"
#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"

#define NUM_INSTANCES                   1000

REGISTER_COMPONENT(sprite)
    REGISTER_COMPONENT_DEFAULT_VALUE(material, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(size, 1.0f)
    REGISTER_COMPONENT_DEFAULT_VALUE(color, float4(1.0f, 1.0f, 1.0f, 1.0f))
    REGISTER_COMPONENT_SERIALIZE(sprite)
        REGISTER_COMPONENT_SERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(size, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(color, StrFromFloat4)
    REGISTER_COMPONENT_DESERIALIZE(sprite)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(size, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(color, StrToFloat4)
END_REGISTER_COMPONENT(sprite)

BEGIN_INTERFACE_LIST(CGEKComponentSystemSprite)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemSprite)

CGEKComponentSystemSprite::CGEKComponentSystemSprite(void)
    : m_pEngine(nullptr)
{
}

CGEKComponentSystemSprite::~CGEKComponentSystemSprite(void)
{
    CGEKObservable::RemoveObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());
    CGEKObservable::RemoveObserver(m_pEngine->GetRenderManager(), (IGEKRenderObserver *)GetUnknown());
}

STDMETHODIMP CGEKComponentSystemSprite::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine->GetRenderManager(), (IGEKRenderObserver *)GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pEngine->GetProgramManager()->LoadProgram(L"sprite", &m_spVertexProgram);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pEngine->GetVideoSystem()->CreateBuffer(sizeof(INSTANCE), NUM_INSTANCES, GEK3DVIDEO::BUFFER::DYNAMIC | GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER | GEK3DVIDEO::BUFFER::RESOURCE, &m_spInstanceBuffer);
    }

    if (SUCCEEDED(hRetVal))
    {
        float2 aVertices[8] =
        {
            float2(-1.0f, -1.0f), float2(0.0f, 0.0f),
            float2( 1.0f, -1.0f), float2(1.0f, 0.0f),
            float2( 1.0f,  1.0f), float2(1.0f, 1.0f),
            float2(-1.0f,  1.0f), float2(0.0f, 1.0f),
        };

        hRetVal = m_pEngine->GetVideoSystem()->CreateBuffer(sizeof(float4), 4, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &m_spVertexBuffer, aVertices);
    }

    if (SUCCEEDED(hRetVal))
    {
        UINT16 aIndices[6] =
        {
            0, 1, 2,
            0, 2, 3,
        };

        hRetVal = m_pEngine->GetVideoSystem()->CreateBuffer(sizeof(UINT16), 6, GEK3DVIDEO::BUFFER::INDEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &m_spIndexBuffer, aIndices);
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemSprite::OnLoadEnd(HRESULT hRetVal)
{
    return S_OK;
}

STDMETHODIMP_(void) CGEKComponentSystemSprite::OnFree(void)
{
}

STDMETHODIMP_(void) CGEKComponentSystemSprite::OnRenderBegin(const GEKENTITYID &nViewerID)
{
}

STDMETHODIMP_(void) CGEKComponentSystemSprite::OnCullScene(const GEKENTITYID &nViewerID, const frustum &nViewFrustum)
{
    m_aVisible.clear();
    m_pEngine->GetSceneManager()->ListComponentsEntities({ GET_COMPONENT_ID(transform), GET_COMPONENT_ID(sprite) }, [&](const GEKENTITYID &nEntityID) -> void
    {
        auto &kSprite = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(sprite)>(nEntityID, GET_COMPONENT_ID(sprite));

        CComPtr<IUnknown> spMaterial;
        m_pEngine->GetMaterialManager()->LoadMaterial(kSprite.material, &spMaterial);
        if (spMaterial)
        {
            float nHalfSize = (kSprite.size * 0.5f);
            auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
            if (nViewFrustum.IsVisible(aabb(kTransform.position - nHalfSize, kTransform.position + nHalfSize)))
            {
                m_aVisible[spMaterial].emplace_back(kTransform.position, nHalfSize, kSprite.color);
            }
        }
    });
}

STDMETHODIMP_(void) CGEKComponentSystemSprite::OnDrawScene(const GEKENTITYID &nViewerID, IGEK3DVideoContext *pContext, UINT32 nVertexAttributes)
{
    REQUIRE_VOID_RETURN(pContext);

    m_pEngine->GetProgramManager()->EnableProgram(pContext, m_spVertexProgram);
    pContext->GetVertexSystem()->SetResource(0, m_spInstanceBuffer);
    pContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);
    pContext->SetVertexBuffer(0, 0, m_spVertexBuffer);
    pContext->SetIndexBuffer(0, m_spIndexBuffer);
    for (auto kModel : m_aVisible)
    {
        for (UINT32 nPass = 0; nPass < kModel.second.size(); nPass += NUM_INSTANCES)
        {
            UINT32 nNumInstances = min(NUM_INSTANCES, (kModel.second.size() - nPass));

            INSTANCE *pInstances = nullptr;
            if (SUCCEEDED(m_spInstanceBuffer->Map((LPVOID *)&pInstances)))
            {
                memcpy(pInstances, &kModel.second[nPass], (sizeof(INSTANCE) * nNumInstances));
                m_spInstanceBuffer->UnMap();

                if (m_pEngine->GetMaterialManager()->EnableMaterial(pContext, kModel.first))
                {
                    pContext->DrawInstancedIndexedPrimitive(6, nNumInstances, 0, 0, 0);
                }
            }
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemSprite::OnRenderEnd(const GEKENTITYID &nViewerID)
{
}
