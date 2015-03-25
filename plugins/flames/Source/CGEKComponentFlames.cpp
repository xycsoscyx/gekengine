#include "CGEKComponentFlames.h"
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

REGISTER_COMPONENT(flames)
    REGISTER_COMPONENT_DEFAULT_VALUE(material, L"flames")
    REGISTER_COMPONENT_DEFAULT_VALUE(gradient, L"flames")
    REGISTER_COMPONENT_DEFAULT_VALUE(color, float4(1.0f, 1.0f, 1.0f, 1.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(color_offset, float4(0.1f, 0.05f, 0.0f, 0.1f))
    REGISTER_COMPONENT_DEFAULT_VALUE(density, 100)
    REGISTER_COMPONENT_DEFAULT_VALUE(life_range, float2(1.5f, 3.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(direction, float3(0.0f, 1.0f, 0.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(direction_offset, float3(20.0f, 0.0f, 20.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(position_offset, float3(0.2f, 0.2f, 0.2f))
    REGISTER_COMPONENT_DEFAULT_VALUE(spin_range, float2(-360.0f, 360.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(size_min_range, float2(0.4f, 0.5f))
    REGISTER_COMPONENT_DEFAULT_VALUE(size_max_range, float2(0.5f, 0.6f))
    REGISTER_COMPONENT_DEFAULT_VALUE(mass_range, float2(0.0f, 0.05f))
    REGISTER_COMPONENT_SERIALIZE(flames)
        REGISTER_COMPONENT_SERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(gradient, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(color, StrFromFloat4)
        REGISTER_COMPONENT_SERIALIZE_VALUE(color_offset, StrFromFloat4)
        REGISTER_COMPONENT_SERIALIZE_VALUE(density, StrFromUINT32)
        REGISTER_COMPONENT_SERIALIZE_VALUE(life_range, StrFromFloat2)
        REGISTER_COMPONENT_SERIALIZE_VALUE(direction, StrFromFloat3)
        REGISTER_COMPONENT_SERIALIZE_VALUE(direction_offset, StrFromFloat3)
        REGISTER_COMPONENT_SERIALIZE_VALUE(position_offset, StrFromFloat3)
        REGISTER_COMPONENT_SERIALIZE_VALUE(spin_range, StrFromFloat2)
        REGISTER_COMPONENT_SERIALIZE_VALUE(size_min_range, StrFromFloat2)
        REGISTER_COMPONENT_SERIALIZE_VALUE(size_max_range, StrFromFloat2)
        REGISTER_COMPONENT_SERIALIZE_VALUE(mass_range, StrFromFloat2)
    REGISTER_COMPONENT_DESERIALIZE(flames)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(gradient, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(color, StrToFloat4)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(color_offset, StrToFloat4)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(density, StrToUINT32)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(life_range, StrToFloat2)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(direction, StrToFloat3)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(direction_offset, StrToFloat3)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(position_offset, StrToFloat3)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(spin_range, StrToFloat2)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(size_min_range, StrToFloat2)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(size_max_range, StrToFloat2)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(mass_range, StrToFloat2)
END_REGISTER_COMPONENT(flames)

BEGIN_INTERFACE_LIST(CGEKComponentSystemFlames)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemFlames)

CGEKComponentSystemFlames::CGEKComponentSystemFlames(void)
    : m_pEngine(nullptr)
{
}

CGEKComponentSystemFlames::~CGEKComponentSystemFlames(void)
{
    CGEKObservable::RemoveObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());
    CGEKObservable::RemoveObserver(m_pEngine->GetRenderManager(), (IGEKRenderObserver *)GetUnknown());
}

STDMETHODIMP CGEKComponentSystemFlames::Initialize(IGEKEngineCore *pEngine)
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

STDMETHODIMP CGEKComponentSystemFlames::OnLoadEnd(HRESULT hRetVal)
{
    return S_OK;
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnFree(void)
{
    m_aEmitters.clear();
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnEntityDestroyed(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aEmitters.find(nEntityID);
    if (pIterator != m_aEmitters.end())
    {
        m_aEmitters.unsafe_erase(pIterator);
    }
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnComponentAdded(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    if (nComponentID == GET_COMPONENT_ID(flames))
    {
        if (m_pEngine->GetSceneManager()->HasComponent(nEntityID, GET_COMPONENT_ID(transform)))
        {
            auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
            auto &kFlames = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(flames)>(nEntityID, GET_COMPONENT_ID(flames));

            EMITTER &kEmitter = m_aEmitters[nEntityID];
            kEmitter.minimum = kTransform.position;
            kEmitter.maximum = kTransform.position;
            kEmitter.m_aParticles.resize(kFlames.density);
            concurrency::parallel_for_each(kEmitter.m_aParticles.begin(), kEmitter.m_aParticles.end(), [&](PARTICLE &kParticle) -> void
            {
                kParticle.m_nLife.y = lerp(kFlames.life_range.x, kFlames.life_range.y, gs_kAbsoluteDistribution(gs_kMersineTwister));
                kParticle.m_nLife.x = (gs_kAbsoluteDistribution(gs_kMersineTwister) * kParticle.m_nLife.y);
                
                kParticle.m_nMass = lerp(kFlames.mass_range.x, kFlames.mass_range.y, gs_kAbsoluteDistribution(gs_kMersineTwister));

                kParticle.m_nSpin.x = _DEGTORAD(gs_kAbsoluteDistribution(gs_kMersineTwister) * 360.0f);
                kParticle.m_nSpin.y = _DEGTORAD(lerp(kFlames.spin_range.x, kFlames.spin_range.y, gs_kAbsoluteDistribution(gs_kMersineTwister)));
                
                kParticle.m_nSize.x = lerp(kFlames.size_min_range.x, kFlames.size_min_range.y, gs_kAbsoluteDistribution(gs_kMersineTwister));
                kParticle.m_nSize.y = lerp(kFlames.size_max_range.x, kFlames.size_max_range.y, gs_kAbsoluteDistribution(gs_kMersineTwister));

                kParticle.m_nPosition = kTransform.position;
                kParticle.m_nPosition.x += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.position_offset.x);
                kParticle.m_nPosition.y += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.position_offset.y);
                kParticle.m_nPosition.z += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.position_offset.z);
                
                quaternion nOffset(_DEGTORAD(gs_kFullDistribution(gs_kMersineTwister) * kFlames.direction_offset.x),
                                   _DEGTORAD(gs_kFullDistribution(gs_kMersineTwister) * kFlames.direction_offset.y),
                                   _DEGTORAD(gs_kFullDistribution(gs_kMersineTwister) * kFlames.direction_offset.z));
                kParticle.m_nVelocity = ((kTransform.rotation * nOffset) * kFlames.direction);

                kParticle.m_nColor = kFlames.color;
                kParticle.m_nColor.r += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.color_offset.r);
                kParticle.m_nColor.g += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.color_offset.g);
                kParticle.m_nColor.b += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.color_offset.b);
                kParticle.m_nColor.a += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.color_offset.a);
            });
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnComponentRemoved(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    if (nComponentID == GET_COMPONENT_ID(flames))
    {
        auto pIterator = m_aEmitters.find(nEntityID);
        if (pIterator != m_aEmitters.end())
        {
            m_aEmitters.unsafe_erase(pIterator);
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnUpdate(float nGameTime, float nFrameTime)
{
    concurrency::parallel_for_each(m_aEmitters.begin(), m_aEmitters.end(), [&](std::pair<const GEKENTITYID, EMITTER> &kPair) -> void
    {
        auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(kPair.first, GET_COMPONENT_ID(transform));
        auto &kFlames = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(flames)>(kPair.first, GET_COMPONENT_ID(flames));

        kPair.second.minimum = _INFINITY;
        kPair.second.maximum =-_INFINITY;
        concurrency::parallel_for_each(kPair.second.m_aParticles.begin(), kPair.second.m_aParticles.end(), [&](PARTICLE &kParticle) -> void
        {
            kParticle.m_nLife.x += nFrameTime;
            kParticle.m_nSpin.x += (kParticle.m_nSpin.y * nFrameTime);
            if (kParticle.m_nLife.x >= kParticle.m_nLife.y)
            {
                kParticle.m_nLife.x = 0.0f;
                kParticle.m_nLife.y = lerp(kFlames.life_range.x, kFlames.life_range.y, gs_kAbsoluteDistribution(gs_kMersineTwister));

                kParticle.m_nPosition = kTransform.position;
                kParticle.m_nPosition.x += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.position_offset.x);
                kParticle.m_nPosition.y += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.position_offset.y);
                kParticle.m_nPosition.z += (gs_kFullDistribution(gs_kMersineTwister) * kFlames.position_offset.z);
                
                quaternion nOffset(_DEGTORAD(gs_kFullDistribution(gs_kMersineTwister) * kFlames.direction_offset.x),
                                   _DEGTORAD(gs_kFullDistribution(gs_kMersineTwister) * kFlames.direction_offset.y),
                                   _DEGTORAD(gs_kFullDistribution(gs_kMersineTwister) * kFlames.direction_offset.z));
                kParticle.m_nVelocity = ((kTransform.rotation * nOffset) * kFlames.direction);
            }
            else
            {
                kParticle.m_nVelocity.y += (-9.81f * kParticle.m_nMass * nFrameTime);
                kParticle.m_nPosition += kParticle.m_nVelocity * nFrameTime;
                kParticle.m_nVelocity.x += (gs_kFullDistribution(gs_kMersineTwister) * nFrameTime);
                kParticle.m_nVelocity.z += (gs_kFullDistribution(gs_kMersineTwister) * nFrameTime);
            }

            kPair.second.Extend(kParticle.m_nPosition);
        });
    });
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnRenderBegin(const GEKENTITYID &nViewerID)
{
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnCullScene(const GEKENTITYID &nViewerID, const frustum &nViewFrustum)
{
    m_aVisible.clear();
    std::for_each(m_aEmitters.begin(), m_aEmitters.end(), [&](std::pair<const GEKENTITYID, EMITTER> &kPair) -> void
    {
        if (nViewFrustum.IsVisible(kPair.second))
        {
            auto &kFlames = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(flames)>(kPair.first, GET_COMPONENT_ID(flames));

            CComPtr<IUnknown> spMaterial;
            m_pEngine->GetMaterialManager()->LoadMaterial(kFlames.material, &spMaterial);

            CComPtr<IGEK3DVideoTexture> spGradient;
            m_pEngine->GetMaterialManager()->LoadTexture(L"%root%\\data\\gradients\\" + kFlames.gradient + L".dds", GEK3DVIDEO::TEXTURE::FORCE_1D, &spGradient);

            if (spMaterial && spGradient)
            {
                concurrency::concurrent_vector<INSTANCE> aVisible;
                aVisible.reserve(kPair.second.m_aParticles.size());
                concurrency::parallel_for_each(kPair.second.m_aParticles.begin(), kPair.second.m_aParticles.end(), [&](PARTICLE &kParticle) -> void
                {
                    float nAge = (kParticle.m_nLife.x / kParticle.m_nLife.y);
                    aVisible.push_back(INSTANCE(kParticle.m_nPosition, nViewFrustum.origin.Distance(kParticle.m_nPosition), nAge, lerp(kParticle.m_nSize.x, kParticle.m_nSize.y, nAge), kParticle.m_nSpin.x, kParticle.m_nColor));
                });

                auto &pVisible = m_aVisible[std::make_pair(spMaterial, spGradient)];
                pVisible.insert(pVisible.end(), aVisible.begin(), aVisible.end());
            }
        }
    });

    for (auto &kMaterial : m_aVisible)
    {
        concurrency::parallel_sort(kMaterial.second.begin(), kMaterial.second.end(), [&](const INSTANCE &kInstanceA, const INSTANCE &kInstanceB) -> bool
        {
            return (kInstanceB.m_nDistance < kInstanceA.m_nDistance);
        });
    }
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnDrawScene(const GEKENTITYID &nViewerID, IGEK3DVideoContext *pContext, UINT32 nVertexAttributes)
{
    REQUIRE_VOID_RETURN(pContext);

    m_pEngine->GetProgramManager()->EnableProgram(pContext, m_spVertexProgram);
    pContext->GetVertexSystem()->SetResource(0, m_spInstanceBuffer);
    pContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);
    pContext->SetVertexBuffer(0, 0, m_spVertexBuffer);
    pContext->SetIndexBuffer(0, m_spIndexBuffer);

    for (auto &kMaterial : m_aVisible)
    {
        if (m_pEngine->GetMaterialManager()->EnableMaterial(pContext, kMaterial.first.first))
        {
            pContext->GetVertexSystem()->SetResource(1, kMaterial.first.second);

            auto &aInstances = kMaterial.second;
            for (UINT32 nPass = 0; nPass < aInstances.size(); nPass += NUM_INSTANCES)
            {
                UINT32 nNumInstances = min(NUM_INSTANCES, (aInstances.size() - nPass));

                INSTANCE *pInstances = nullptr;
                if (SUCCEEDED(m_spInstanceBuffer->Map((LPVOID *)&pInstances)))
                {
                    memcpy(pInstances, &aInstances[nPass], (sizeof(INSTANCE) * nNumInstances));
                    m_spInstanceBuffer->UnMap();

                    pContext->DrawInstancedIndexedPrimitive(6, nNumInstances, 0, 0, 0);
                }
            }
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemFlames::OnRenderEnd(const GEKENTITYID &nViewerID)
{
}
