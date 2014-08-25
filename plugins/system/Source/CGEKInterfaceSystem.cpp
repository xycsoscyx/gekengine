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
    : m_pVideoSystem(nullptr)
    , m_pVideoContext(nullptr)
{
}

CGEKInterfaceSystem::~CGEKInterfaceSystem(void)
{
}

STDMETHODIMP CGEKInterfaceSystem::Initialize(void)
{
    GEKFUNCTION(nullptr);
    
    HRESULT hRetVal = E_FAIL;
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    if (m_pVideoSystem)
    {
        m_spDevice = m_pVideoSystem;
        if (m_spDevice)
        {
            hRetVal = GetContext()->AddCachedClass(CLSID_GEKInterfaceSystem, GetUnknown());
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = FW1CreateFactory(FW1_VERSION, &m_spFontFactory);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float4x4), 1, GEKVIDEO::BUFFER::CONSTANT_BUFFER, &m_spOrthoBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
        if (m_spOrthoBuffer)
        {
            float4x4 nOverlayMatrix;
            nOverlayMatrix.SetOrthographic(0.0f, 0.0f, 640.0f, 480.0f, -1.0f, 1.0f);
            m_spOrthoBuffer->Update((void *)&nOverlayMatrix);
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer((sizeof(float4) * 2), 4, GEKVIDEO::BUFFER::VERTEX_BUFFER | GEKVIDEO::BUFFER::DYNAMIC, &m_spVertexBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        UINT16 aIndices[6] =
        {
            0, 1, 2,
            0, 2, 3,
        };

        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT16), 6, GEKVIDEO::BUFFER::INDEX_BUFFER | GEKVIDEO::BUFFER::STATIC, &m_spIndexBuffer, aIndices);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        std::vector<GEKVIDEO::INPUTELEMENT> aLayout;
        aLayout.push_back(GEKVIDEO::INPUTELEMENT(GEKVIDEO::DATA::RG_FLOAT, "POSITION", 0));
        aLayout.push_back(GEKVIDEO::INPUTELEMENT(GEKVIDEO::DATA::RG_FLOAT, "TEXCOORD", 0));
        aLayout.push_back(GEKVIDEO::INPUTELEMENT(GEKVIDEO::DATA::RGBA_FLOAT, "COLOR", 0));
        hRetVal = m_pVideoSystem->LoadVertexProgram(L"%root%\\data\\programs\\vertex\\interface.hlsl", "MainVertexProgram", aLayout, &m_spVertexProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to LoadVertexProgram failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->LoadPixelProgram(L"%root%\\data\\programs\\pixel\\interface.hlsl", "MainPixelProgram", &m_spPixelProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to LoadPixelProgram failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::SAMPLERSTATES kStates;
        kStates.m_eFilter = GEKVIDEO::FILTER::MIN_MAG_MIP_POINT;
        kStates.m_eAddressU = GEKVIDEO::ADDRESS::CLAMP;
        kStates.m_eAddressV = GEKVIDEO::ADDRESS::CLAMP;
        hRetVal = m_pVideoSystem->CreateSamplerStates(kStates, &m_spPointSampler);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateSamplerStates failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::RENDERSTATES kRenderStates;
        hRetVal = m_pVideoSystem->CreateRenderStates(kRenderStates, &m_spRenderStates);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::UNIFIEDBLENDSTATES kBlendStates;
        hRetVal = m_pVideoSystem->CreateBlendStates(kBlendStates, &m_spBlendStates);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKVIDEO::DEPTHSTATES kDepthStates;
        hRetVal = m_pVideoSystem->CreateDepthStates(kDepthStates, &m_spDepthStates);
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
    m_pVideoContext = pContext;
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

STDMETHODIMP_(void) CGEKInterfaceSystem::DrawSprite(const GEKVIDEO::RECT<float> &kRect, const GEKVIDEO::RECT<float> &kTexCoords, const float4 &nColor, IGEKVideoTexture *pTexture)
{
    REQUIRE_VOID_RETURN(m_pVideoContext);
    
    struct VERTEX
    {
        float2 position;
        float2 texcoord;
        float4 color;
    } *pVertices = nullptr;

    m_spVertexBuffer->Map((LPVOID *)&pVertices);
    pVertices[0].position = float2(kRect.left, kRect.top);
    pVertices[1].position = float2(kRect.right, kRect.top);
    pVertices[2].position = float2(kRect.right, kRect.bottom);
    pVertices[3].position = float2(kRect.left, kRect.bottom);
    pVertices[0].texcoord = float2(kTexCoords.left, kTexCoords.top);
    pVertices[1].texcoord = float2(kTexCoords.right, kTexCoords.top);
    pVertices[2].texcoord = float2(kTexCoords.right, kTexCoords.bottom);
    pVertices[3].texcoord = float2(kTexCoords.left, kTexCoords.bottom);
    pVertices[0].color = nColor;
    pVertices[1].color = nColor;
    pVertices[2].color = nColor;
    pVertices[3].color = nColor;
    m_spVertexBuffer->UnMap();

    m_pVideoContext->SetBlendStates(float4(1.0f), 0xFFFFFFFF, m_spBlendStates);
    m_pVideoContext->SetRenderStates(m_spRenderStates);
    m_pVideoContext->SetDepthStates(0x0, m_spDepthStates);
    m_pVideoContext->GetComputeSystem()->SetProgram(nullptr);
    m_pVideoContext->GetVertexSystem()->SetProgram(m_spVertexProgram);
    m_pVideoContext->GetVertexSystem()->SetConstantBuffer(0, m_spOrthoBuffer);
    m_pVideoContext->GetGeometrySystem()->SetProgram(nullptr);
    m_pVideoContext->GetPixelSystem()->SetProgram(m_spPixelProgram);
    m_pVideoContext->GetPixelSystem()->SetSamplerStates(0, m_spPointSampler);
    m_pVideoContext->GetPixelSystem()->SetResource(0, pTexture ? pTexture : nullptr);
    m_pVideoContext->SetVertexBuffer(0, 0, m_spVertexBuffer);
    m_pVideoContext->SetIndexBuffer(0, m_spIndexBuffer);
    m_pVideoContext->SetPrimitiveType(GEKVIDEO::PRIMITIVE::TRIANGLELIST);
    m_pVideoContext->DrawIndexedPrimitive(6, 0, 0);
}
