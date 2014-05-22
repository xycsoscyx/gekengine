#include "CGEKPlayerState.h"
#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKPlayerState)
    INTERFACE_LIST_ENTRY_COM(IGEKLogicState)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKPlayerState)

CGEKPlayerState::CGEKPlayerState(void)
    : m_pEntity(nullptr)
    , m_bActive(true)
{
}

CGEKPlayerState::~CGEKPlayerState(void)
{
}

STDMETHODIMP_(void) CGEKPlayerState::OnEnter(IGEKEntity *pEntity)
{
    m_pEntity = pEntity;
    IGEKViewManager *pViewManager = GetContext()->GetCachedClass<IGEKViewManager>(CLSID_GEKRenderManager);
    if (pViewManager != nullptr)
    {
        pViewManager->SetViewer(m_pEntity);
        pViewManager->CaptureMouse(true);
    }
}

STDMETHODIMP_(void) CGEKPlayerState::OnExit(void)
{
    m_pEntity = nullptr;
}

STDMETHODIMP_(void) CGEKPlayerState::OnEvent(LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB)
{
    if (_wcsicmp(pAction, L"input") == 0)
    {
        if (kParamA.GetString().CompareNoCase(L"escape") == 0)
        {
            if (!kParamB.GetBoolean())
            {
                m_bActive = !m_bActive;
                IGEKViewManager *pViewManager = GetContext()->GetCachedClass<IGEKViewManager>(CLSID_GEKRenderManager);
                if (pViewManager != nullptr)
                {
                    pViewManager->CaptureMouse(m_bActive);
                }
            }
        }
        else if (m_pEntity != nullptr)
        {
            if (kParamA.GetString().CompareNoCase(L"turn") == 0)
            {
                m_nRotation += kParamB.GetFloat2() * 0.01f;
            }
            else
            {
                m_aActions[kParamA.GetString()] = kParamB.GetBoolean();
            }
        }
    }
}

STDMETHODIMP_(void) CGEKPlayerState::OnUpdate(float nGameTime, float nFrameTime)
{
    IGEKComponent *pTransform = m_pEntity->GetComponent(L"transform");
    if (pTransform != nullptr)
    {
        float3 nForce;
        float4x4 nRotation = quaternion(m_nRotation.y, m_nRotation.x, 0.0f);
        if (m_aActions[L"forward"])
        {
            nForce += nRotation.rz;
        }
        
        if (m_aActions[L"backward"])
        {
            nForce -= nRotation.rz;
        }
        
        if (m_aActions[L"strafe_right"])
        {
            nForce += nRotation.rx;
        }
        
        if (m_aActions[L"strafe_left"])
        {
            nForce -= nRotation.rx;
        }
        
        if (m_aActions[L"rise"])
        {
            nForce += nRotation.ry;
        }
        
        if (m_aActions[L"fall"])
        {
            nForce -= nRotation.ry;
        }

        nForce *= 10.0f;
        GEKVALUE kPosition;
        pTransform->GetProperty(L"position", kPosition);
        float3 nPosition = (kPosition.GetFloat3() + (nForce * nFrameTime));
        pTransform->SetProperty(L"position", nPosition);
        pTransform->SetProperty(L"rotation", quaternion(nRotation));
    }
}
