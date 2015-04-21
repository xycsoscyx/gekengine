#include "CGEKComponentParticle.h"
#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"
#include "GEKAPI.h"
#include <random>
#include <ppl.h>

static std::random_device gs_kRandomDevice;
static std::mt19937 gs_kMersineTwister(gs_kRandomDevice());
static std::uniform_real_distribution<float> gs_kFullDistribution(-1.0f, 1.0f);
static std::uniform_real_distribution<float> gs_kAbsoluteDistribution(0.0f, 1.0f);

#define NUM_INSTANCES                   10000

BEGIN_INTERFACE_LIST(CGEKComponentSystemParticle)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemParticle)

CGEKComponentSystemParticle::CGEKComponentSystemParticle(void)
    : m_pEngine(nullptr)
{
}

CGEKComponentSystemParticle::~CGEKComponentSystemParticle(void)
{
    CGEKObservable::RemoveObserver(m_pEngine->GetRenderManager(), GetClass<IGEKRenderObserver>());
}

STDMETHODIMP CGEKComponentSystemParticle::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine->GetRenderManager(), GetClass<IGEKRenderObserver>());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pEngine->GetProgramManager()->LoadProgram(L"flames", &m_spVertexProgram);
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

STDMETHODIMP_(void) CGEKComponentSystemParticle::OnRenderBegin(const GEKENTITYID &nViewerID)
{
}

STDMETHODIMP_(void) CGEKComponentSystemParticle::OnCullScene(const GEKENTITYID &nViewerID, const frustum &nViewFrustum)
{
}

STDMETHODIMP_(void) CGEKComponentSystemParticle::OnDrawScene(const GEKENTITYID &nViewerID, IGEK3DVideoContext *pContext, UINT32 nVertexAttributes)
{
}

STDMETHODIMP_(void) CGEKComponentSystemParticle::OnRenderEnd(const GEKENTITYID &nViewerID)
{
}
