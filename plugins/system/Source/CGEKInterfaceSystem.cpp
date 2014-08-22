#include "CGEKInterfaceSystem.h"
#include "IGEKAudioSystem.h"
#include "IGEKVideoSystem.h"
#include <time.h>

#include "GEKSystemCLSIDs.h"

#pragma comment(lib, "FW1FontWrapper.lib")

BEGIN_INTERFACE_LIST(CGEKInterfaceSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKInterfaceSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKInterfaceSystem);

CGEKInterfaceSystem::CGEKInterfaceSystem(void)
{
}

CGEKInterfaceSystem::~CGEKInterfaceSystem(void)
{
}

STDMETHODIMP CGEKInterfaceSystem::Initialize(void)
{
    GEKFUNCTION(nullptr);
    
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKInterfaceSystem, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        IGEKVideoSystem *pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
        if (pVideoSystem)
        {
            m_spDevice = pVideoSystem;
            if (m_spDevice)
            {
                hRetVal = FW1CreateFactory(FW1_VERSION, &m_spFontFactory);
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKInterfaceSystem::Destroy(void)
{
    GetContext()->RemoveCachedClass(CLSID_GEKInterfaceSystem);
}

STDMETHODIMP CGEKInterfaceSystem::SetContext(IGEKVideoContext *pContext)
{
    REQUIRE_RETURN(m_spFontFactory, E_FAIL);
    REQUIRE_RETURN(pContext, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    m_spDeviceContext = pContext;
    if (m_spDeviceContext)
    {
        hRetVal = m_spFontFactory->CreateFontWrapper(m_spDevice, L"Arial", &m_spFontWrapper);
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKInterfaceSystem::Print(LPCWSTR pFont, float nSize, UINT32 nColor, const GEKVIDEO::RECT<float> &aLayoutRect, const GEKVIDEO::RECT<float> &kClipRect, LPCWSTR pFormat, ...)
{
    REQUIRE_VOID_RETURN(m_spDeviceContext && m_spFontWrapper && pFont && pFormat);

    CStringW strMessage;

    va_list pArgs;
    va_start(pArgs, pFormat);
    strMessage.FormatV(pFormat, pArgs);
    va_end(pArgs);

    m_spFontWrapper->DrawString(m_spDeviceContext, strMessage, pFont, nSize, (FW1_RECTF *)&aLayoutRect, nColor, (FW1_RECTF *)&kClipRect, nullptr, 0);
}
