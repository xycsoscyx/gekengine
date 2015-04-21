#include "CGEKComponentGravity.h"
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

BEGIN_INTERFACE_LIST(CGEKComponentSystemGravity)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemGravity)

CGEKComponentSystemGravity::CGEKComponentSystemGravity(void)
    : m_pEngine(nullptr)
{
}

CGEKComponentSystemGravity::~CGEKComponentSystemGravity(void)
{
    CGEKObservable::RemoveObserver(m_pEngine->GetSceneManager(), GetClass<IGEKSceneObserver>());
}

STDMETHODIMP CGEKComponentSystemGravity::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = hRetVal = CGEKObservable::AddObserver(m_pEngine->GetSceneManager(), GetClass<IGEKSceneObserver>());
    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemGravity::OnLoadEnd(HRESULT hRetVal)
{
    return S_OK;
}

STDMETHODIMP_(void) CGEKComponentSystemGravity::OnFree(void)
{
}

STDMETHODIMP_(void) CGEKComponentSystemGravity::OnEntityCreated(const GEKENTITYID &nEntityID)
{
}

STDMETHODIMP_(void) CGEKComponentSystemGravity::OnEntityDestroyed(const GEKENTITYID &nEntityID)
{
}

STDMETHODIMP_(void) CGEKComponentSystemGravity::OnUpdate(float nGameTime, float nFrameTime)
{
}
