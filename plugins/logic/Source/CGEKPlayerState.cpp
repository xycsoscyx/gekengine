#include "CGEKPlayerState.h"

BEGIN_INTERFACE_LIST(CGEKPlayerState)
    INTERFACE_LIST_ENTRY_COM(IGEKLogicSystemUser)
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
    CComQIPtr<IGEKViewManager> spViewManager(GetLogicSystem());
    if (spViewManager)
    {
        spViewManager->CaptureMouse(true);
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
                CComQIPtr<IGEKViewManager> spViewManager(GetLogicSystem());
                if (spViewManager)
                {
                    spViewManager->CaptureMouse(m_bActive);
                }
            }
        }
        else if (m_pEntity != nullptr)
        {
            if (kParamA.GetString().CompareNoCase(L"turn") == 0)
            {
                m_nRotation += kParamB.GetFloat2() * 3.0f;
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
    if (pTransform)
    {
        GEKVALUE kPosition;
        GEKVALUE kRotation;
        pTransform->GetProperty(L"position", kPosition);
        pTransform->GetProperty(L"rotation", kRotation);

        float3 nForce;
        float4x4 nRotation = kRotation.GetQuaternion();
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

        nForce *= 5.0f;
        float3 nPosition = (kPosition.GetFloat3() + (nForce * nFrameTime));
        pTransform->SetProperty(L"position", nPosition);

        nRotation = (nRotation * quaternion(0.0, m_nRotation.x * nFrameTime, 0.0));
        pTransform->SetProperty(L"rotation", quaternion(nRotation));
    }

    m_nRotation = 0.0f;
}
