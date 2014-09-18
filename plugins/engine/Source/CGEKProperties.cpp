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

HRESULT CGEKRenderStates::Load(IGEKVideoSystem *pSystem, CLibXMLNode &kStatesNode)
{
    GEKVIDEO::RENDERSTATES kRenderStates;
    if (kStatesNode.HasAttribute(L"fillmode"))
    {
        kRenderStates.m_eFillMode = GetFillMode(kStatesNode.GetAttribute(L"fillmode"));
    }

    if (kStatesNode.HasAttribute(L"cullmode"))
    {
        kRenderStates.m_eCullMode = GetCullMode(kStatesNode.GetAttribute(L"cullmode"));
    }

    if (kStatesNode.HasAttribute(L"frontcounterclockwize"))
    {
        kRenderStates.m_bFrontCounterClockwise = StrToBoolean(kStatesNode.GetAttribute(L"frontcounterclockwize"));
    }

    if (kStatesNode.HasAttribute(L"depthbias"))
    {
        kRenderStates.m_nDepthBias = StrToUINT32(kStatesNode.GetAttribute(L"depthbias"));
    }

    if (kStatesNode.HasAttribute(L"depthbiasclamp"))
    {
        kRenderStates.m_nDepthBiasClamp = StrToFloat(kStatesNode.GetAttribute(L"depthbiasclamp"));
    }

    if (kStatesNode.HasAttribute(L"slopescaleddepthbias"))
    {
        kRenderStates.m_nSlopeScaledDepthBias = StrToFloat(kStatesNode.GetAttribute(L"slopescaleddepthbias"));
    }

    if (kStatesNode.HasAttribute(L"depthclip"))
    {
        kRenderStates.m_bDepthClipEnable = StrToBoolean(kStatesNode.GetAttribute(L"depthclip"));
    }

    if (kStatesNode.HasAttribute(L"multisample"))
    {
        kRenderStates.m_bMultisampleEnable = StrToBoolean(kStatesNode.GetAttribute(L"multisample"));
    }

    return pSystem->CreateRenderStates(kRenderStates, &m_spRenderStates);
}

void CGEKRenderStates::Enable(IGEKVideoContext *pContext)
{
    REQUIRE_VOID_RETURN(pContext);
    pContext->SetRenderStates(m_spRenderStates);
}

CGEKBlendStates::~CGEKBlendStates(void)
{
}

void GetTargetStates(GEKVIDEO::TARGETBLENDSTATES &kStates, CLibXMLNode &kTargetNode)
{
    CLibXMLNode kColorNode = kTargetNode.FirstChildElement(L"color");
    CLibXMLNode kAlphaNode = kTargetNode.FirstChildElement(L"alpha");
    if (kColorNode && kAlphaNode)
    {
        if (kColorNode.HasAttribute(L"source") && kColorNode.HasAttribute(L"destination") && kColorNode.HasAttribute(L"operation") &&
            kAlphaNode.HasAttribute(L"source") && kAlphaNode.HasAttribute(L"destination") && kAlphaNode.HasAttribute(L"operation"))
        {
            kStates.m_bEnable = true;
            kStates.m_eColorSource = GetBlendFactor(kColorNode.GetAttribute(L"source"));
            kStates.m_eColorDestination = GetBlendFactor(kColorNode.GetAttribute(L"destination"));
            kStates.m_eColorOperation = GetBlendOperation(kColorNode.GetAttribute(L"operation"));
            kStates.m_eAlphaSource = GetBlendFactor(kAlphaNode.GetAttribute(L"source"));
            kStates.m_eAlphaDestination = GetBlendFactor(kAlphaNode.GetAttribute(L"destination"));
            kStates.m_eAlphaOperation = GetBlendOperation(kAlphaNode.GetAttribute(L"operation"));
        }
    }

    if (kTargetNode.HasAttribute(L"writemask"))
    {
        CStringW strMask(kTargetNode.GetAttribute(L"writemask"));
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
}

HRESULT CGEKBlendStates::Load(IGEKVideoSystem *pSystem, CLibXMLNode &kBlendNode)
{
    if (kBlendNode.HasAttribute(L"factor"))
    {
        m_nBlendFactor = StrToFloat4(kBlendNode.GetAttribute(L"factor"));
    }
    else
    {
        m_nBlendFactor = 1.0f;
    }

    if (kBlendNode.HasAttribute(L"mask"))
    {
        m_nSampleMask = StrToUINT32(kBlendNode.GetAttribute(L"mask"));
    }
    else
    {
        m_nSampleMask = 0xFFFFFFFF;
    }

    bool bAlphaToCoverage = false;
    if (kBlendNode.HasAttribute(L"alphatocoverage"))
    {
        bAlphaToCoverage = StrToBoolean(kBlendNode.GetAttribute(L"alphatocoverage"));
    }

    HRESULT hRetVal = E_FAIL;
    if (kBlendNode.HasAttribute(L"independent") && StrToBoolean(kBlendNode.GetAttribute(L"independent")))
    {
        UINT32 nTarget = 0;
        GEKVIDEO::INDEPENDENTBLENDSTATES kBlendStates;
        kBlendStates.m_bAlphaToCoverage = bAlphaToCoverage;
        CLibXMLNode &kTargetNode = kBlendNode.FirstChildElement(L"target");
        while (kTargetNode)
        {
            GetTargetStates(kBlendStates.m_aTargetStates[nTarget++], kTargetNode);
            kTargetNode = kTargetNode.NextSiblingElement(L"target");
        };

        hRetVal = pSystem->CreateBlendStates(kBlendStates, &m_spBlendStates);

    }
    else
    {
        GEKVIDEO::UNIFIEDBLENDSTATES kBlendStates;
        kBlendStates.m_bAlphaToCoverage = bAlphaToCoverage;
        GetTargetStates(kBlendStates, kBlendNode);
        hRetVal = pSystem->CreateBlendStates(kBlendStates, &m_spBlendStates);
    }

    return hRetVal;
}

void CGEKBlendStates::Enable(IGEKVideoContext *pContext)
{
    REQUIRE_VOID_RETURN(pContext);
    pContext->SetBlendStates(m_nBlendFactor, m_nSampleMask, m_spBlendStates);
}
