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

        CLibXMLNode kTarget = kTargets.FirstChildElement(L"target");
        while (kTarget)
        {
            GEKVIDEO::DATA::FORMAT eFormat = GetTargetFormat(kTarget.GetAttribute(L"format"));

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
    return CGEKRenderStates::Load(GetVideoSystem(), kFilter.FirstChildElement(L"render"));
}

HRESULT CGEKRenderFilter::LoadBlendStates(CLibXMLNode &kFilter)
{
    return CGEKBlendStates::Load(GetVideoSystem(), kFilter.FirstChildElement(L"blend"));
}

HRESULT CGEKRenderFilter::LoadProgram(CLibXMLNode &kFilter)
{
    HRESULT hRetVal = E_INVALIDARG;
    CLibXMLNode kPixel = kFilter.FirstChildElement(L"pixel");
    if (kPixel)
    {
        CStringW strVertexAttributes = kPixel.GetAttribute(L"vertexattributes");
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

        CStringW strMode = kPixel.GetAttribute(L"mode");
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
                CStringA strCoreProgram = kPixel.GetText();
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
                hRetVal = LoadProgram(kFilter);
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

STDMETHODIMP_(IGEKVideoRenderStates *) CGEKRenderFilter::GetRenderStates(void)
{
    return m_spRenderStates;
}

STDMETHODIMP_(IGEKVideoBlendStates *) CGEKRenderFilter::GetBlendStates(void)
{
    return m_spBlendStates;
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
    
    CGEKRenderStates::Enable(GetVideoSystem());
    CGEKBlendStates::Enable(GetVideoSystem());
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