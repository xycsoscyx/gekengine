#include "CGEKProperties.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

GEKVIDEO::FILL::MODE GetFillMode(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"solid") == 0) return GEKVIDEO::FILL::SOLID;
    else if (_wcsicmp(pValue, L"wire") == 0) return GEKVIDEO::FILL::WIREFRAME;
    else return GEKVIDEO::FILL::SOLID;
}

GEKVIDEO::CULL::MODE GetCullMode(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"none") == 0) return GEKVIDEO::CULL::NONE;
    else if (_wcsicmp(pValue, L"front") == 0) return GEKVIDEO::CULL::FRONT;
    else if (_wcsicmp(pValue, L"back") == 0) return GEKVIDEO::CULL::BACK;
    else return GEKVIDEO::CULL::NONE;
}

GEKVIDEO::BLEND::FACTOR::SOURCE GetBlendFactor(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"zero") == 0) return GEKVIDEO::BLEND::FACTOR::ZERO;
    else if (_wcsicmp(pValue, L"one") == 0) return GEKVIDEO::BLEND::FACTOR::ONE;
    else if (_wcsicmp(pValue, L"blend_factor") == 0) return GEKVIDEO::BLEND::FACTOR::BLENDFACTOR;
    else if (_wcsicmp(pValue, L"inverse_blend_factor") == 0) return GEKVIDEO::BLEND::FACTOR::INVERSE_BLENDFACTOR;
    else if (_wcsicmp(pValue, L"source_color") == 0) return GEKVIDEO::BLEND::FACTOR::SOURCE_COLOR;
    else if (_wcsicmp(pValue, L"inverse_source_color") == 0) return GEKVIDEO::BLEND::FACTOR::INVERSE_SOURCE_COLOR;
    else if (_wcsicmp(pValue, L"source_alpha") == 0) return GEKVIDEO::BLEND::FACTOR::SOURCE_ALPHA;
    else if (_wcsicmp(pValue, L"inverse_source_alpha") == 0) return GEKVIDEO::BLEND::FACTOR::INVERSE_SOURCE_ALPHA;
    else if (_wcsicmp(pValue, L"source_alpha_saturate") == 0) return GEKVIDEO::BLEND::FACTOR::SOURCE_ALPHA_SATURATE;
    else if (_wcsicmp(pValue, L"destination_color") == 0) return GEKVIDEO::BLEND::FACTOR::DESTINATION_COLOR;
    else if (_wcsicmp(pValue, L"inverse_destination_color") == 0) return GEKVIDEO::BLEND::FACTOR::INVERSE_DESTINATION_COLOR;
    else if (_wcsicmp(pValue, L"destination_alpha") == 0) return GEKVIDEO::BLEND::FACTOR::DESTINATION_ALPHA;
    else if (_wcsicmp(pValue, L"inverse_destination_alpha") == 0) return GEKVIDEO::BLEND::FACTOR::INVERSE_DESTINATION_ALPHA;
    else if (_wcsicmp(pValue, L"secondary_source_color") == 0) return GEKVIDEO::BLEND::FACTOR::SECONRARY_SOURCE_COLOR;
    else if (_wcsicmp(pValue, L"inverse_secondary_source_color") == 0) return GEKVIDEO::BLEND::FACTOR::INVERSE_SECONRARY_SOURCE_COLOR;
    else if (_wcsicmp(pValue, L"secondary_source_alpha") == 0) return GEKVIDEO::BLEND::FACTOR::SECONRARY_SOURCE_ALPHA;
    else if (_wcsicmp(pValue, L"inverse_secondary_source_alpha") == 0) return GEKVIDEO::BLEND::FACTOR::INVERSE_SECONRARY_SOURCE_ALPHA;
    else return GEKVIDEO::BLEND::FACTOR::ZERO;
}

GEKVIDEO::BLEND::OPERATION GetBlendOperation(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"add") == 0) return GEKVIDEO::BLEND::ADD;
    else if (_wcsicmp(pValue, L"subtract") == 0) return GEKVIDEO::BLEND::SUBTRACT;
    else if (_wcsicmp(pValue, L"reverse_subtract") == 0) return GEKVIDEO::BLEND::REVERSE_SUBTRACT;
    else if (_wcsicmp(pValue, L"minimum") == 0) return GEKVIDEO::BLEND::MINIMUM;
    else if (_wcsicmp(pValue, L"maximum") == 0) return GEKVIDEO::BLEND::MAXIMUM;
    else return GEKVIDEO::BLEND::ADD;
}

CGEKRenderStates::~CGEKRenderStates(void)
{
}

HRESULT CGEKRenderStates::Load(IGEKVideoSystem *pSystem, CLibXMLNode &kStates)
{
    GEKVIDEO::RENDERSTATES kRenderStates;
    if (kStates.HasAttribute(L"fillmode"))
    {
        kRenderStates.m_eFillMode = GetFillMode(kStates.GetAttribute(L"fillmode"));
    }

    if (kStates.HasAttribute(L"cullmode"))
    {
        kRenderStates.m_eCullMode = GetCullMode(kStates.GetAttribute(L"cullmode"));
    }

    if (kStates.HasAttribute(L"frontcounterclockwize"))
    {
        kRenderStates.m_bFrontCounterClockwise = StrToBoolean(kStates.GetAttribute(L"frontcounterclockwize"));
    }

    if (kStates.HasAttribute(L"depthbias"))
    {
        kRenderStates.m_nDepthBias = StrToUINT32(kStates.GetAttribute(L"depthbias"));
    }

    if (kStates.HasAttribute(L"depthbiasclamp"))
    {
        kRenderStates.m_nDepthBiasClamp = StrToFloat(kStates.GetAttribute(L"depthbiasclamp"));
    }

    if (kStates.HasAttribute(L"slopescaleddepthbias"))
    {
        kRenderStates.m_nSlopeScaledDepthBias = StrToFloat(kStates.GetAttribute(L"slopescaleddepthbias"));
    }

    if (kStates.HasAttribute(L"depthclip"))
    {
        kRenderStates.m_bDepthClipEnable = StrToBoolean(kStates.GetAttribute(L"depthclip"));
    }

    if (kStates.HasAttribute(L"multisample"))
    {
        kRenderStates.m_bMultisampleEnable = StrToBoolean(kStates.GetAttribute(L"multisample"));
    }

    return pSystem->CreateRenderStates(kRenderStates, &m_spRenderStates);
}

void CGEKRenderStates::Enable(IGEKVideoSystem *pSystem)
{
    pSystem->GetImmediateContext()->SetRenderStates(m_spRenderStates);
}

CGEKBlendStates::~CGEKBlendStates(void)
{
}

void GetTargetStates(GEKVIDEO::TARGETBLENDSTATES &kStates, CLibXMLNode &kNode)
{
    if (kNode.HasAttribute(L"writemask"))
    {
        kStates.m_bEnable = true;
        CStringW strMask(kNode.GetAttribute(L"writemask"));
        strMask.MakeLower();

        kStates.m_nWriteMask = 0;
        if (strMask.Find(L"r") >= 0) kStates.m_nWriteMask |= GEKVIDEO::COLOR::R;
        if (strMask.Find(L"g") >= 0) kStates.m_nWriteMask |= GEKVIDEO::COLOR::G;
        if (strMask.Find(L"b") >= 0) kStates.m_nWriteMask |= GEKVIDEO::COLOR::B;
        if (strMask.Find(L"a") >= 0) kStates.m_nWriteMask |= GEKVIDEO::COLOR::A;
    }
    else
    {
        kStates.m_nWriteMask = GEKVIDEO::COLOR::RGBA;
    }

    CLibXMLNode kColor = kNode.FirstChildElement(L"color");
    CLibXMLNode kAlpha = kNode.FirstChildElement(L"alpha");
    if (kColor && kAlpha)
    {
        if (kColor.HasAttribute(L"source") && kColor.HasAttribute(L"destination") && kColor.HasAttribute(L"operation") &&
            kAlpha.HasAttribute(L"source") && kAlpha.HasAttribute(L"destination") && kAlpha.HasAttribute(L"operation"))
        {
            kStates.m_bEnable = true;
            kStates.m_eColorSource = GetBlendFactor(kColor.GetAttribute(L"source"));
            kStates.m_eColorDestination = GetBlendFactor(kColor.GetAttribute(L"destination"));
            kStates.m_eColorOperation = GetBlendOperation(kColor.GetAttribute(L"operation"));
            kStates.m_eAlphaSource = GetBlendFactor(kAlpha.GetAttribute(L"source"));
            kStates.m_eAlphaDestination = GetBlendFactor(kAlpha.GetAttribute(L"destination"));
            kStates.m_eAlphaOperation = GetBlendOperation(kAlpha.GetAttribute(L"operation"));
        }
    }
}

HRESULT CGEKBlendStates::Load(IGEKVideoSystem *pSystem, CLibXMLNode &kBlend)
{
    bool bAlphaToCoverage = false;
    if (kBlend.HasAttribute(L"alphatocoverage") && StrToBoolean(kBlend.GetAttribute(L"alphatocoverage")))
    {
        bAlphaToCoverage = true;
    }

    if (kBlend.HasAttribute(L"factor"))
    {
        m_nBlendFactor = StrToFloat4(kBlend.GetAttribute(L"factor"));
    }

    HRESULT hRetVal = E_FAIL;
    if (kBlend.HasAttribute(L"independent"))
    {
        UINT32 nTarget = 0;
        GEKVIDEO::INDEPENDENTBLENDSTATES kBlendStates;
        CLibXMLNode &kTarget = kBlend.FirstChildElement(L"target");
        while (kTarget)
        {
            GetTargetStates(kBlendStates.m_aTargetStates[nTarget++], kTarget);
            kTarget = kTarget.NextSiblingElement(L"target");
        };

        kBlendStates.m_bAlphaToCoverage = bAlphaToCoverage;
        hRetVal = pSystem->CreateBlendStates(kBlendStates, &m_spBlendStates);

    }
    else
    {
        GEKVIDEO::UNIFIEDBLENDSTATES kBlendStates;
        GetTargetStates(kBlendStates, kBlend);
        kBlendStates.m_bAlphaToCoverage = bAlphaToCoverage;
        hRetVal = pSystem->CreateBlendStates(kBlendStates, &m_spBlendStates);
    }

    return hRetVal;
}

void CGEKBlendStates::Enable(IGEKVideoSystem *pSystem)
{
    pSystem->GetImmediateContext()->SetBlendStates(m_nBlendFactor, 0xFFFFFFFF, m_spBlendStates);
}
