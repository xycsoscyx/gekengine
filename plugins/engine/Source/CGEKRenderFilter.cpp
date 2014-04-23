#include "CGEKRenderFilter.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"

GEKVIDEO::DATA::FORMAT GetTargetFormat(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"RGBA_FLOAT") == 0) return GEKVIDEO::DATA::RGBA_FLOAT;
    else if (_wcsicmp(pValue, L"RGBA_UINT8") == 0) return GEKVIDEO::DATA::RGBA_UINT8;
    else if (_wcsicmp(pValue, L"D16") == 0) return GEKVIDEO::DATA::D16;
    else if (_wcsicmp(pValue, L"D32") == 0) return GEKVIDEO::DATA::D32;
    else if (_wcsicmp(pValue, L"D24S8") == 0) return GEKVIDEO::DATA::D24_S8;
    else return GEKVIDEO::DATA::UNKNOWN;
}

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

GEKVIDEO::DEPTHWRITE::MASK GetDepthWriteMask(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"zero") == 0) return GEKVIDEO::DEPTHWRITE::ZERO;
    else if (_wcsicmp(pValue, L"all") == 0) return GEKVIDEO::DEPTHWRITE::ALL;
    else return GEKVIDEO::DEPTHWRITE::ZERO;
}

GEKVIDEO::COMPARISON::FUNCTION GetComparisonFunction(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"always") == 0) return GEKVIDEO::COMPARISON::ALWAYS;
    else if (_wcsicmp(pValue, L"never") == 0) return GEKVIDEO::COMPARISON::NEVER;
    else if (_wcsicmp(pValue, L"equal") == 0) return GEKVIDEO::COMPARISON::EQUAL;
    else if (_wcsicmp(pValue, L"notequal") == 0) return GEKVIDEO::COMPARISON::NOT_EQUAL;
    else if (_wcsicmp(pValue, L"less") == 0) return GEKVIDEO::COMPARISON::LESS;
    else if (_wcsicmp(pValue, L"lessequal") == 0) return GEKVIDEO::COMPARISON::LESS_EQUAL;
    else if (_wcsicmp(pValue, L"greater") == 0) return GEKVIDEO::COMPARISON::GREATER;
    else if (_wcsicmp(pValue, L"greaterequal") == 0) return GEKVIDEO::COMPARISON::GREATER_EQUAL;
    else return GEKVIDEO::COMPARISON::ALWAYS;
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

GEKVIDEO::STENCIL::OPERATION GetStencilOperation(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"ZERO") == 0) return GEKVIDEO::STENCIL::ZERO;
    else if (_wcsicmp(pValue, L"KEEP") == 0) return GEKVIDEO::STENCIL::KEEP;
    else if (_wcsicmp(pValue, L"REPLACE") == 0) return GEKVIDEO::STENCIL::REPLACE;
    else if (_wcsicmp(pValue, L"INVERT") == 0) return GEKVIDEO::STENCIL::INVERT;
    else if (_wcsicmp(pValue, L"INCREASE") == 0) return GEKVIDEO::STENCIL::INCREASE;
    else if (_wcsicmp(pValue, L"INCREASE_SATURATED") == 0) return GEKVIDEO::STENCIL::INCREASE_SATURATED;
    else if (_wcsicmp(pValue, L"DECREASE") == 0) return GEKVIDEO::STENCIL::DECREASE;
    else if (_wcsicmp(pValue, L"DECREASE_SATURATED") == 0) return GEKVIDEO::STENCIL::DECREASE_SATURATED;
    else return GEKVIDEO::STENCIL::ZERO;
}

void GetStentilStates(GEKVIDEO::STENCILSTATES &kStates, CLibXMLNode &kNode)
{
    if (kNode.HasAttribute(L"pass"))
    {
        kStates.m_eStencilPassOperation = GetStencilOperation(kNode.GetAttribute(L"pass"));
    }
            
    if (kNode.HasAttribute(L"fail"))
    {
        kStates.m_eStencilFailOperation = GetStencilOperation(kNode.GetAttribute(L"fail"));
    }

    if (kNode.HasAttribute(L"depthfail"))
    {
        kStates.m_eStencilDepthFailOperation = GetStencilOperation(kNode.GetAttribute(L"depthfail"));
    }

    if (kNode.HasAttribute(L"comparison"))
    {
        kStates.m_eStencilComparison = GetComparisonFunction(kNode.GetAttribute(L"comparison"));
    }
}

BEGIN_INTERFACE_LIST(CGEKRenderFilter)
    INTERFACE_LIST_ENTRY_COM(IGEKSystemUser)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoSystemUser)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderFilter)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderFilter)

CGEKRenderFilter::CGEKRenderFilter(void)
    : m_nScale(1.0f)
    , m_eDepthFormat(GEKVIDEO::DATA::UNKNOWN)
    , m_nVertexAttributes(0xFFFFFFFF)
    , m_eMode(STANDARD)
    , m_nBlendFactor(1.0f)
    , m_bClearDepth(false)
    , m_bClearStencil(false)
    , m_nClearDepth(0.0f)
    , m_nClearStencil(0)
    , m_nStencilReference(0)
{
}

CGEKRenderFilter::~CGEKRenderFilter(void)
{
}

STDMETHODIMP CGEKRenderFilter::Initialize(void)
{
    return CGEKObservable::AddObserver(GetVideoSystem(), this);
}

STDMETHODIMP_(void) CGEKRenderFilter::Destroy(void)
{
    CGEKObservable::RemoveObserver(GetVideoSystem(), this);
}

STDMETHODIMP_(void) CGEKRenderFilter::OnPreReset(void)
{
    for (auto &kTarget : m_aTargetList)
    {
        kTarget.m_spTexture = nullptr;
    }

    m_spDepthBuffer = nullptr;
}

STDMETHODIMP CGEKRenderFilter::OnPostReset(void)
{
    HRESULT hRetVal = S_OK;
    UINT32 nXSize = UINT32(float(GetSystem()->GetXSize()) * m_nScale);
    UINT32 nYSize = UINT32(float(GetSystem()->GetYSize()) * m_nScale);
    for (auto &kTarget : m_aTargetList)
    {
        if (kTarget.m_eFormat != GEKVIDEO::DATA::UNKNOWN)
        {
            hRetVal = GetVideoSystem()->CreateRenderTarget(nXSize, nYSize, kTarget.m_eFormat, &kTarget.m_spTexture);
            if (FAILED(hRetVal))
            {
                break;
            }
        }
    }

    if (SUCCEEDED(hRetVal) && m_eDepthFormat != GEKVIDEO::DATA::UNKNOWN)
    {
        hRetVal = GetVideoSystem()->CreateDepthTarget(nXSize, nYSize, m_eDepthFormat, &m_spDepthBuffer);
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadDepthStates(CLibXMLNode &kTargets, UINT32 nXSize, UINT32 nYSize)
{
    HRESULT hRetVal = S_OK;
    GEKVIDEO::DEPTHSTATES kDepthStates;
    CLibXMLNode kDepth = kTargets.FirstChildElement(L"depth");
    if (kDepth)
    {
        if (kDepth.HasAttribute(L"source"))
        {
            m_strDepthSource = kDepth.GetAttribute(L"source");
            hRetVal = S_OK;
        }
        else
        {
            GEKVIDEO::DATA::FORMAT eFormat = GEKVIDEO::DATA::D32;
            if (kDepth.HasAttribute(L"format"))
            {
                eFormat = GetTargetFormat(kDepth.GetAttribute(L"format"));
            }

            if (eFormat == GEKVIDEO::DATA::UNKNOWN)
            {
                hRetVal = E_INVALIDARG;
            }
            else
            {
                m_eDepthFormat = eFormat;
                hRetVal = GetVideoSystem()->CreateDepthTarget(nXSize, nYSize, eFormat, &m_spDepthBuffer);
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            kDepthStates.m_bDepthEnable = true;
            if (kDepth.HasAttribute(L"clear"))
            {
                m_bClearDepth = true;
                m_nClearDepth = StrToFloat(kDepth.GetAttribute(L"clear"));
            }

            if (kDepth.HasAttribute(L"comparison"))
            {
                kDepthStates.m_eDepthComparison = GetComparisonFunction(kDepth.GetAttribute(L"comparison"));
            }

            if (kDepth.HasAttribute(L"writemask"))
            {
                kDepthStates.m_eDepthWriteMask = GetDepthWriteMask(kDepth.GetAttribute(L"writemask"));
            }
        }

        CLibXMLNode kStencil = kDepth.FirstChildElement(L"stencil");
        if (kStencil)
        {
            kDepthStates.m_bStencilEnable = true;
            if (kStencil.HasAttribute(L"clear"))
            {
                m_bClearStencil = true;
                m_nClearStencil = StrToUINT32(kStencil.GetAttribute(L"clear"));
            }

            if (kStencil.HasAttribute(L"reference"))
            {
                m_nStencilReference = StrToUINT32(kStencil.GetAttribute(L"reference"));
            }

            if (kStencil.HasAttribute(L"readmask"))
            {
                kDepthStates.m_nStencilReadMask = StrToUINT32(kStencil.GetAttribute(L"readmask"));
            }

            if (kStencil.HasAttribute(L"writemask"))
            {
                kDepthStates.m_nStencilReadMask = StrToUINT32(kStencil.GetAttribute(L"writemask"));
            }

            CLibXMLNode kFront = kStencil.FirstChildElement(L"front");
            if (kFront)
            {
                GetStentilStates(kDepthStates.m_kStencilFrontStates, kFront);
            }

            CLibXMLNode kBack = kStencil.FirstChildElement(L"back");
            if (kBack)
            {
                GetStentilStates(kDepthStates.m_kStencilBackStates, kBack);
            }
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetVideoSystem()->CreateDepthStates(kDepthStates, &m_spDepthStates);
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadTargets(CLibXMLNode &kFilter)
{
    HRESULT hRetVal = S_OK;
    CLibXMLNode kTargets = kFilter.FirstChildElement(L"targets");
    if (kTargets)
    {
        m_nScale = 1.0f;
        if (kTargets.HasAttribute(L"scale"))
        {
            m_nScale = StrToFloat(kTargets.GetAttribute(L"scale"));
        }

        UINT32 nXSize = UINT32(float(GetSystem()->GetXSize()) * m_nScale);
        UINT32 nYSize = UINT32(float(GetSystem()->GetYSize()) * m_nScale);
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = LoadDepthStates(kTargets, nXSize, nYSize);
        }

        GEKVIDEO::DATA::FORMAT eFormat = GetTargetFormat(kTargets.GetAttribute(L"format"));
        CLibXMLNode kTarget = kTargets.FirstChildElement(L"target");
        while (kTarget)
        {
            TARGET kData;
            if (kTarget.HasAttribute(L"clear"))
            {
                kData.m_bClear = true;
                kData.m_nClearColor = StrToFloat4(kTarget.GetAttribute(L"clear"));
            }
            else
            {
                kData.m_bClear = false;
            }

            if (kTarget.HasAttribute(L"name"))
            {
                if (eFormat == GEKVIDEO::DATA::UNKNOWN)
                {
                    hRetVal = E_INVALIDARG;
                    break;
                }

                CComPtr<IGEKVideoTexture> spTexture;
                hRetVal = GetVideoSystem()->CreateRenderTarget(nXSize, nYSize, eFormat, &spTexture);
                if (spTexture != nullptr)
                {
                    kData.m_eFormat = eFormat;
                    kData.m_spTexture = spTexture;
                    m_aTargetList.push_back(kData);
                    m_aTargetMap[kTarget.GetAttribute(L"name")] = &m_aTargetList.back();
                }
                else
                {
                    break;
                }
            }
            else if (kTarget.HasAttribute(L"source"))
            {
                kData.m_strSource = kTarget.GetAttribute(L"source");
                m_aTargetList.push_back(kData);
            }
            else
            {
                hRetVal = E_INVALIDARG;
                break;
            }

            kTarget = kTarget.NextSiblingElement(L"target");
        }
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadTextures(CLibXMLNode &kFilter)
{
    HRESULT hRetVal = S_OK;
    CLibXMLNode kTextures = kFilter.FirstChildElement(L"textures");
    if (kTextures)
    {
        CLibXMLNode kTexture = kTextures.FirstChildElement(L"texture");
        while (kTexture)
        {
            SOURCE kSource;
            if (kTexture.HasAttribute(L"source"))
            {
                kSource.m_strName = kTexture.GetAttribute(L"source");
            }
            else if (kTexture.HasAttribute(L"data"))
            {
                CComPtr<IUnknown> spTexture;
                hRetVal = GetRenderManager()->LoadTexture(kTexture.GetAttribute(L"data"), &spTexture);
                if (SUCCEEDED(hRetVal))
                {
                    kSource.m_spTexture = spTexture;
                }
                else
                {
                    break;
                }
            }
            else
            {
                hRetVal = E_INVALIDARG;
                break;
            }

            m_aSourceList.push_back(kSource);
            kTexture = kTexture.NextSiblingElement(L"texture");
        };
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadRenderStates(CLibXMLNode &kFilter)
{
    HRESULT hRetVal = S_OK;
    GEKVIDEO::RENDERSTATES kRenderStates;
    CLibXMLNode kStates = kFilter.FirstChildElement(L"render");
    if (kStates)
    {
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
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetVideoSystem()->CreateRenderStates(kRenderStates, &m_spRenderStates);
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadBlendStates(CLibXMLNode &kFilter)
{
    HRESULT hRetVal = S_OK;
    GEKVIDEO::UNIFIEDBLENDSTATES kBlendStates;
    CLibXMLNode kBlend = kFilter.FirstChildElement(L"blend");
    if (kBlend)
    {
        if (kBlend.HasAttribute(L"factor"))
        {
            m_nBlendFactor = StrToFloat(kBlend.GetAttribute(L"factor"));
        }

        if (kBlend.HasAttribute(L"writemask"))
        {
            kBlendStates.m_bEnable = true;
            CStringW strMask(kBlend.GetAttribute(L"writemask"));
            strMask.MakeLower();

            kBlendStates.m_nWriteMask = 0;
            if (strMask.Find(L"r") >= 0) kBlendStates.m_nWriteMask |= GEKVIDEO::COLOR::R;
            if (strMask.Find(L"g") >= 0) kBlendStates.m_nWriteMask |= GEKVIDEO::COLOR::G;
            if (strMask.Find(L"b") >= 0) kBlendStates.m_nWriteMask |= GEKVIDEO::COLOR::B;
            if (strMask.Find(L"a") >= 0) kBlendStates.m_nWriteMask |= GEKVIDEO::COLOR::A;
        }
        else
        {
            kBlendStates.m_nWriteMask = GEKVIDEO::COLOR::RGBA;
        }

        CLibXMLNode kColor = kBlend.FirstChildElement(L"color");
        CLibXMLNode kAlpha = kBlend.FirstChildElement(L"alpha");
        if (kColor && kAlpha)
        {
            if (kColor.HasAttribute(L"source") && kColor.HasAttribute(L"destination") && kColor.HasAttribute(L"operation") &&
                kAlpha.HasAttribute(L"source") && kAlpha.HasAttribute(L"destination") && kAlpha.HasAttribute(L"operation"))
            {
                kBlendStates.m_bEnable = true;
                kBlendStates.m_eColorSource = GetBlendFactor(kColor.GetAttribute(L"source"));
                kBlendStates.m_eColorDestination = GetBlendFactor(kColor.GetAttribute(L"destination"));
                kBlendStates.m_eColorOperation = GetBlendOperation(kColor.GetAttribute(L"operation"));
                kBlendStates.m_eAlphaSource = GetBlendFactor(kAlpha.GetAttribute(L"source"));
                kBlendStates.m_eAlphaDestination = GetBlendFactor(kAlpha.GetAttribute(L"destination"));
                kBlendStates.m_eAlphaOperation = GetBlendOperation(kAlpha.GetAttribute(L"operation"));
            }
            else
            {
                hRetVal = E_INVALIDARG;
            }
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetVideoSystem()->CreateBlendStates(kBlendStates, &m_spBlendStates);
    }

    return hRetVal;
}

HRESULT CGEKRenderFilter::LoadShader(CLibXMLNode &kFilter)
{
    HRESULT hRetVal = E_INVALIDARG;
    CLibXMLNode kProgram = kFilter.FirstChildElement(L"program");
    if (kProgram)
    {
        CStringW strVertexAttributes = kProgram.GetAttribute(L"vertexattributes");
        if (!strVertexAttributes.IsEmpty())
        {
            m_nVertexAttributes = 0;

            int nPosition = 0;
            CStringW strAttribute = strVertexAttributes.Tokenize(L",", nPosition);
            while (!strAttribute.IsEmpty())
            {
                if (strAttribute.CompareNoCase(L"position") == 0)
                {
                    m_nVertexAttributes |= GEK_VERTEX_POSITION;
                }
                else if (strAttribute.CompareNoCase(L"position") == 0)
                {
                    m_nVertexAttributes |= GEK_VERTEX_TEXCOORD;
                }
                else if (strAttribute.CompareNoCase(L"position") == 0)
                {
                    m_nVertexAttributes |= GEK_VERTEX_BASIS;
                }

                strAttribute = strVertexAttributes.Tokenize(L",", nPosition);
            };
        }
        else
        {
            m_nVertexAttributes = 0xFFFFFFFF;
        }

        CStringW strMode = kProgram.GetAttribute(L"mode");
        if (strMode.CompareNoCase(L"forward") == 0)
        {
            m_eMode = FORWARD;
        }
        else if (strMode.CompareNoCase(L"lighting") == 0)
        {
            m_eMode = LIGHTING;
        }
        else
        {
            hRetVal = E_INVALIDARG;
        }

        CStringA strProgram;
        switch (m_eMode)
        {
        case FORWARD:
            hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\pixel\\forward.txt", strProgram);
            break;

        case LIGHTING:
            hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\pixel\\lighting.txt", strProgram);
            break;

        default:
            hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\pixel\\default.txt", strProgram);
            break;
        };

        if (SUCCEEDED(hRetVal))
        {
            if (strProgram.Find("_INSERT_PIXEL_PROGRAM") < 0)
            {
                hRetVal = E_INVALIDARG;
            }
            else
            {
                CStringA strCoreProgram = kProgram.GetText();
                strProgram.Replace("_INSERT_PIXEL_PROGRAM", strCoreProgram);
                hRetVal = GetVideoSystem()->CompilePixelProgram(strProgram, "MainPixelProgram", &m_spPixelProgram);
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderFilter::Load(LPCWSTR pFileName)
{
    CLibXMLDoc kDocument;
    HRESULT hRetVal = kDocument.Load(pFileName);
    if (SUCCEEDED(hRetVal))
    {
        CLibXMLNode kFilter = kDocument.GetRoot();
        if (kFilter)
        {
            hRetVal = LoadBlendStates(kFilter);
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadRenderStates(kFilter);
            }

            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadTargets(kFilter);
            }
                
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadTextures(kFilter);
            }
                
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = LoadShader(kFilter);
            }

            if (SUCCEEDED(hRetVal) && !m_spDepthStates)
            {
                GEKVIDEO::DEPTHSTATES kDepthStates;
                hRetVal = GetVideoSystem()->CreateDepthStates(kDepthStates, &m_spDepthStates);
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(UINT32) CGEKRenderFilter::GetVertexAttributes(void)
{
    return m_nVertexAttributes;
}

STDMETHODIMP CGEKRenderFilter::GetBuffer(LPCWSTR pName, IUnknown **ppTexture)
{
    REQUIRE_RETURN(ppTexture, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aTargetMap.find(pName);
    if (pIterator != m_aTargetMap.end())
    {
        hRetVal = ((*pIterator).second)->m_spTexture->QueryInterface(IID_PPV_ARGS(ppTexture));
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderFilter::GetDepthBuffer(IUnknown **ppBuffer)
{
    HRESULT hRetVal = E_FAIL;
    if (m_spDepthBuffer != nullptr)
    {
        hRetVal = m_spDepthBuffer->QueryInterface(IID_PPV_ARGS(ppBuffer));
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderFilter::Draw(void)
{
    CComPtr<IUnknown> spDepthBuffer;
    if (m_spDepthBuffer != nullptr)
    {
        spDepthBuffer = m_spDepthBuffer;
    }
    else if (!m_strDepthSource.IsEmpty())
    {
        GetRenderManager()->GetDepthBuffer(m_strDepthSource, &spDepthBuffer);
    }

    if (spDepthBuffer != nullptr)
    {
        DWORD nFlags = 0;
        if (m_bClearDepth)
        {
            nFlags |= GEKVIDEO::CLEAR::DEPTH;
        }

        if (m_bClearStencil)
        {
            nFlags |= GEKVIDEO::CLEAR::STENCIL;
        }

        if (nFlags > 0)
        {
            GetVideoSystem()->GetDefaultContext()->ClearDepthStencilTarget(spDepthBuffer, nFlags, m_nClearDepth, m_nClearStencil);
        }
    }

    std::vector<IGEKVideoTexture *> aTargets;
    if (m_aTargetList.size() > 0)
    {
        for (auto &kTarget : m_aTargetList)
        {
            CComQIPtr<IGEKVideoTexture> spTarget;
            if (kTarget.m_spTexture != nullptr)
            {
                spTarget = kTarget.m_spTexture;
            }
            else if (!kTarget.m_strSource.IsEmpty())
            {
                CComPtr<IUnknown> spUnknown;
                GetRenderManager()->GetBuffer(kTarget.m_strSource, &spUnknown);
                if (spUnknown)
                {
                    spTarget = spUnknown;
                }
            }

            if (spTarget != nullptr)
            {
                if (kTarget.m_bClear)
                {
                    GetVideoSystem()->GetDefaultContext()->ClearRenderTarget(spTarget, kTarget.m_nClearColor);
                }

                aTargets.push_back(spTarget);
            }
        }

        GetVideoSystem()->GetDefaultContext()->SetRenderTargets(aTargets, (spDepthBuffer ? spDepthBuffer : nullptr));
    }
    else
    {
        GetVideoSystem()->SetDefaultTargets(nullptr, (spDepthBuffer ? spDepthBuffer : nullptr));
    }

    UINT32 nStage = 0;
    for (auto &kSource : m_aSourceList)
    {
        if (kSource.m_spTexture != nullptr)
        {
            GetRenderManager()->SetTexture(nStage, kSource.m_spTexture);
        }
        else if (!kSource.m_strName.IsEmpty())
        {
            CComPtr<IUnknown> spTexture;
            GetRenderManager()->GetBuffer(kSource.m_strName, &spTexture);
            if (spTexture != nullptr)
            {
                GetRenderManager()->SetTexture(nStage, spTexture);
            }
        }

        nStage++;
    }
    
    GetVideoSystem()->GetDefaultContext()->SetRenderStates(m_spRenderStates);
    GetVideoSystem()->GetDefaultContext()->SetBlendStates(m_nBlendFactor, 0xFFFFFFFF, m_spBlendStates);
    GetVideoSystem()->GetDefaultContext()->SetDepthStates(m_nStencilReference, m_spDepthStates);
    GetVideoSystem()->GetDefaultContext()->SetPixelProgram(m_spPixelProgram);
    if (m_eMode == FORWARD)
    {
        GetRenderManager()->DrawScene(0xFFFFFFFF);
    }
    else if (m_eMode == LIGHTING)
    {
        GetRenderManager()->DrawOverlay(true);
    }
    else
    {
        GetRenderManager()->DrawOverlay(false);
    }
}