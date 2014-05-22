#include "CGEKLightMoveState.h"
#include <random>

BEGIN_INTERFACE_LIST(CGEKLightMoveState)
    INTERFACE_LIST_ENTRY_COM(IGEKLogicState)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKLightMoveState)

CGEKLightMoveState::CGEKLightMoveState(void)
    : m_pEntity(nullptr)
{
}

CGEKLightMoveState::~CGEKLightMoveState(void)
{
}

STDMETHODIMP_(void) CGEKLightMoveState::OnEnter(IGEKEntity *pEntity)
{
    m_pEntity = pEntity;
    if (m_pEntity != nullptr)
    {
        IGEKComponent *pTransform = m_pEntity->GetComponent(L"transform");
        if (pTransform != nullptr)
        {
            GEKVALUE kPosition;
            pTransform->GetProperty(L"position", kPosition);
            m_nOrigin = kPosition.GetFloat3();

            static std::random_device kRandomDevice;
            static std::mt19937 kMersine(kRandomDevice());
            static std::uniform_real_distribution<float> kRandom(1.0, 2.0);
            m_nOffset = (kRandom(kMersine) * _2_PI);
            m_nSpeed = (kRandom(kMersine) * 2.0f);
            m_nSize = (kRandom(kMersine) * 5.0f);
        }
    }
}

STDMETHODIMP_(void) CGEKLightMoveState::OnExit(void)
{
    m_pEntity = nullptr;
}

STDMETHODIMP_(void) CGEKLightMoveState::OnUpdate(float nGameTime, float nFrameTime)
{
    IGEKComponent *pTransform = m_pEntity->GetComponent(L"transform");
    if (pTransform != nullptr)
    {
        float3 nOffset;
        nOffset.x = sin(nGameTime * m_nSpeed + m_nOffset);
        nOffset.y = sin(nGameTime * m_nSpeed + m_nOffset + _PI_2);
        nOffset.z = cos(nGameTime * m_nSpeed + m_nOffset);

        float3 nPosition(m_nOrigin + nOffset * m_nSize);
        pTransform->SetProperty(L"position", nPosition);
    }
}
