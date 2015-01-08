#include "CGEKProperties.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

GEK3DVIDEO::FILL::MODE GetFillMode(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"solid") == 0) return GEK3DVIDEO::FILL::SOLID;
    else if (_wcsicmp(pValue, L"wire") == 0) return GEK3DVIDEO::FILL::WIREFRAME;
    else return GEK3DVIDEO::FILL::SOLID;
}

GEK3DVIDEO::CULL::MODE GetCullMode(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"none") == 0) return GEK3DVIDEO::CULL::NONE;
    else if (_wcsicmp(pValue, L"front") == 0) return GEK3DVIDEO::CULL::FRONT;
    else if (_wcsicmp(pValue, L"back") == 0) return GEK3DVIDEO::CULL::BACK;
    else return GEK3DVIDEO::CULL::NONE;
}

GEK3DVIDEO::BLEND::FACTOR::SOURCE GetBlendFactor(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"zero") == 0) return GEK3DVIDEO::BLEND::FACTOR::ZERO;
    else if (_wcsicmp(pValue, L"one") == 0) return GEK3DVIDEO::BLEND::FACTOR::ONE;
    else if (_wcsicmp(pValue, L"blend_factor") == 0) return GEK3DVIDEO::BLEND::FACTOR::BLENDFACTOR;
    else if (_wcsicmp(pValue, L"inverse_blend_factor") == 0) return GEK3DVIDEO::BLEND::FACTOR::INVERSE_BLENDFACTOR;
    else if (_wcsicmp(pValue, L"source_color") == 0) return GEK3DVIDEO::BLEND::FACTOR::SOURCE_COLOR;
    else if (_wcsicmp(pValue, L"inverse_source_color") == 0) return GEK3DVIDEO::BLEND::FACTOR::INVERSE_SOURCE_COLOR;
    else if (_wcsicmp(pValue, L"source_alpha") == 0) return GEK3DVIDEO::BLEND::FACTOR::SOURCE_ALPHA;
    else if (_wcsicmp(pValue, L"inverse_source_alpha") == 0) return GEK3DVIDEO::BLEND::FACTOR::INVERSE_SOURCE_ALPHA;
    else if (_wcsicmp(pValue, L"source_alpha_saturate") == 0) return GEK3DVIDEO::BLEND::FACTOR::SOURCE_ALPHA_SATURATE;
    else if (_wcsicmp(pValue, L"destination_color") == 0) return GEK3DVIDEO::BLEND::FACTOR::DESTINATION_COLOR;
    else if (_wcsicmp(pValue, L"inverse_destination_color") == 0) return GEK3DVIDEO::BLEND::FACTOR::INVERSE_DESTINATION_COLOR;
    else if (_wcsicmp(pValue, L"destination_alpha") == 0) return GEK3DVIDEO::BLEND::FACTOR::DESTINATION_ALPHA;
    else if (_wcsicmp(pValue, L"inverse_destination_alpha") == 0) return GEK3DVIDEO::BLEND::FACTOR::INVERSE_DESTINATION_ALPHA;
    else if (_wcsicmp(pValue, L"secondary_source_color") == 0) return GEK3DVIDEO::BLEND::FACTOR::SECONRARY_SOURCE_COLOR;
    else if (_wcsicmp(pValue, L"inverse_secondary_source_color") == 0) return GEK3DVIDEO::BLEND::FACTOR::INVERSE_SECONRARY_SOURCE_COLOR;
    else if (_wcsicmp(pValue, L"secondary_source_alpha") == 0) return GEK3DVIDEO::BLEND::FACTOR::SECONRARY_SOURCE_ALPHA;
    else if (_wcsicmp(pValue, L"inverse_secondary_source_alpha") == 0) return GEK3DVIDEO::BLEND::FACTOR::INVERSE_SECONRARY_SOURCE_ALPHA;
    else return GEK3DVIDEO::BLEND::FACTOR::ZERO;
}

GEK3DVIDEO::BLEND::OPERATION GetBlendOperation(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"add") == 0) return GEK3DVIDEO::BLEND::ADD;
    else if (_wcsicmp(pValue, L"subtract") == 0) return GEK3DVIDEO::BLEND::SUBTRACT;
    else if (_wcsicmp(pValue, L"reverse_subtract") == 0) return GEK3DVIDEO::BLEND::REVERSE_SUBTRACT;
    else if (_wcsicmp(pValue, L"minimum") == 0) return GEK3DVIDEO::BLEND::MINIMUM;
    else if (_wcsicmp(pValue, L"maximum") == 0) return GEK3DVIDEO::BLEND::MAXIMUM;
    else return GEK3DVIDEO::BLEND::ADD;
}

CGEKRenderStates::~CGEKRenderStates(void)
{
}

HRESULT CGEKRenderStates::Load(IGEK3DVideoSystem *pSystem, CLibXMLNode &kStatesNode)
{
    REQUIRE_RETURN(pSystem, E_INVALIDARG);
    GEK3DVIDEO::RENDERSTATES kRenderStates;
    if (kStatesNode.HasAttribute(L"fillmode"))
    {
        kRenderStates.m_eFillMode = GetFillMode(kStatesNode.GetAttribute(L"fillmode"));
    }

    if (kStatesNode.HasAttribute(L"cullmode"))
    {
        kRenderStates.m_eCullMode = GetCullMode(kStatesNode.GetAttribute(L"cullmode"));
    }

    if (kStatesNode.HasAttribute(L"frontcounterclockwise"))
    {
        kRenderStates.m_bFrontCounterClockwise = StrToBoolean(kStatesNode.GetAttribute(L"frontcounterclockwise"));
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

void CGEKRenderStates::Enable(IGEK3DVideoContext *pContext)
{
    REQUIRE_VOID_RETURN(pContext);
    if (m_spRenderStates)
    {
        pContext->SetRenderStates(m_spRenderStates);
    }
}

CGEKBlendStates::~CGEKBlendStates(void)
{
}

void GetTargetStates(GEK3DVIDEO::TARGETBLENDSTATES &kStates, CLibXMLNode &kTargetNode)
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
        if (strMask.Find(L"r") >= 0) kStates.m_nWriteMask |= GEK3DVIDEO::COLOR::R;
        if (strMask.Find(L"g") >= 0) kStates.m_nWriteMask |= GEK3DVIDEO::COLOR::G;
        if (strMask.Find(L"b") >= 0) kStates.m_nWriteMask |= GEK3DVIDEO::COLOR::B;
        if (strMask.Find(L"a") >= 0) kStates.m_nWriteMask |= GEK3DVIDEO::COLOR::A;
    }
    else
    {
        kStates.m_nWriteMask = GEK3DVIDEO::COLOR::RGBA;
    }
}

HRESULT CGEKBlendStates::Load(IGEK3DVideoSystem *pSystem, CLibXMLNode &kBlendNode)
{
    REQUIRE_RETURN(pSystem, E_INVALIDARG);
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
        GEK3DVIDEO::INDEPENDENTBLENDSTATES kBlendStates;
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
        GEK3DVIDEO::UNIFIEDBLENDSTATES kBlendStates;
        kBlendStates.m_bAlphaToCoverage = bAlphaToCoverage;
        GetTargetStates(kBlendStates, kBlendNode);
        hRetVal = pSystem->CreateBlendStates(kBlendStates, &m_spBlendStates);
    }

    return hRetVal;
}

void CGEKBlendStates::Enable(IGEK3DVideoContext *pContext)
{
    REQUIRE_VOID_RETURN(pContext);
    if (m_spBlendStates)
    {
        pContext->SetBlendStates(m_nBlendFactor, m_nSampleMask, m_spBlendStates);
    }
}
