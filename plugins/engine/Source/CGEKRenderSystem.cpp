#include "CGEKRenderSystem.h"
#include "IGEKRenderFilter.h"
#include "CGEKProperties.h"
#include <windowsx.h>
#include <atlpath.h>
#include <algorithm>
#include <concurrent_vector.h>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

DECLARE_INTERFACE_IID_(IGEKMaterial, IUnknown, "819CA201-F652-4183-B29D-BB71BB15810E")
{
    STDMETHOD_(void, Enable)                (THIS_ CGEKRenderSystem *pManager, IGEK3DVideoContext *pContext) PURE;
    STDMETHOD_(float4, GetColor)            (THIS) PURE;
    STDMETHOD_(bool, IsFullBright)          (THIS) PURE;
};

class CGEKMaterial : public CGEKUnknown
                   , public IGEKMaterial
                   , public CGEKRenderStates
                   , public CGEKBlendStates
{
private:
    CComPtr<IUnknown> m_spAlbedoMap;
    CComPtr<IUnknown> m_spNormalMap;
    CComPtr<IUnknown> m_spInfoMap;
    bool m_bFullBright;
    float4 m_nColor;

public:
    DECLARE_UNKNOWN(CGEKMaterial);
    CGEKMaterial(IUnknown *pAlbedoMap, IUnknown *pNormalMap, IUnknown *pInfoMap, const float4 &nColor, bool bFullBright)
        : m_spAlbedoMap(pAlbedoMap)
        , m_spNormalMap(pNormalMap)
        , m_spInfoMap(pInfoMap)
        , m_nColor(nColor)
        , m_bFullBright(bFullBright)
    {
    }

    ~CGEKMaterial(void)
    {
    }

    // IGEKMaterial
    STDMETHODIMP_(void) Enable(CGEKRenderSystem *pManager, IGEK3DVideoContext *pContext)
    {
        pManager->SetResource(pContext->GetPixelSystem(), 0, m_spAlbedoMap);
        pManager->SetResource(pContext->GetPixelSystem(), 1, m_spNormalMap);
        pManager->SetResource(pContext->GetPixelSystem(), 2, m_spInfoMap);
        CGEKRenderStates::Enable(pContext);
        CGEKBlendStates::Enable(pContext);
    }

    STDMETHODIMP_(float4) GetColor(void)
    {
        return m_nColor;
    }

    STDMETHODIMP_(bool) IsFullBright(void)
    {
        return m_bFullBright;
    }
};

BEGIN_INTERFACE_LIST(CGEKMaterial)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterial)
END_INTERFACE_LIST_UNKNOWN

DECLARE_INTERFACE_IID_(IGEKProgram, IUnknown, "0387E446-E858-4F3C-9E19-1F0E36D914E3")
{
    STDMETHOD_(IUnknown *, GetVertexProgram)    (THIS) PURE;
    STDMETHOD_(IUnknown *, GetGeometryProgram)  (THIS) PURE;
};

class CGEKProgram : public CGEKUnknown
                  , public IGEKProgram
{
private:
    CComPtr<IUnknown> m_spVertexProgram;
    CComPtr<IUnknown> m_spGeometryProgram;

public:
    DECLARE_UNKNOWN(CGEKProgram);
    CGEKProgram(IUnknown *pVertexProgram, IUnknown *pGeometryProgram)
        : m_spVertexProgram(pVertexProgram)
        , m_spGeometryProgram(pGeometryProgram)
    {
    }

    ~CGEKProgram(void)
    {
    }

    STDMETHODIMP_(IUnknown *) GetVertexProgram(void)
    {
        return m_spVertexProgram;
    }

    STDMETHODIMP_(IUnknown *) GetGeometryProgram(void)
    {
        return (m_spGeometryProgram ? m_spGeometryProgram : nullptr);
    }
};

BEGIN_INTERFACE_LIST(CGEKProgram)
    INTERFACE_LIST_ENTRY_COM(IGEKProgram)
END_INTERFACE_LIST_UNKNOWN

static GEK3DVIDEO::DATA::FORMAT GetFormatType(LPCWSTR pValue)
{
         if (_wcsicmp(pValue, L"R_FLOAT") == 0) return GEK3DVIDEO::DATA::R_FLOAT;
    else if (_wcsicmp(pValue, L"RG_FLOAT") == 0) return GEK3DVIDEO::DATA::RG_FLOAT;
    else if (_wcsicmp(pValue, L"RGB_FLOAT") == 0) return GEK3DVIDEO::DATA::RGB_FLOAT;
    else if (_wcsicmp(pValue, L"RGBA_FLOAT") == 0) return GEK3DVIDEO::DATA::RGBA_FLOAT;
    else if (_wcsicmp(pValue, L"R_UINT32") == 0) return GEK3DVIDEO::DATA::R_UINT32;
    else if (_wcsicmp(pValue, L"RG_UINT32") == 0) return GEK3DVIDEO::DATA::RG_UINT32;
    else if (_wcsicmp(pValue, L"RGB_UINT32") == 0) return GEK3DVIDEO::DATA::RGB_UINT32;
    else if (_wcsicmp(pValue, L"RGBA_UINT32") == 0) return GEK3DVIDEO::DATA::RGBA_UINT32;
    else return GEK3DVIDEO::DATA::UNKNOWN;
}

static GEK3DVIDEO::INPUT::SOURCE GetElementClass(LPCWSTR pValue)
{
         if (_wcsicmp(pValue, L"vertex") == 0) return GEK3DVIDEO::INPUT::VERTEX;
    else if (_wcsicmp(pValue, L"instance") == 0) return GEK3DVIDEO::INPUT::INSTANCE;
    else return GEK3DVIDEO::INPUT::UNKNOWN;
}

BEGIN_INTERFACE_LIST(CGEKRenderSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKProgramManager)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterialManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderSystem)

CGEKRenderSystem::CGEKRenderSystem(void)
    : m_pSystem(nullptr)
    , m_pVideoSystem(nullptr)
    , m_pEngine(nullptr)
    , m_pSceneManager(nullptr)
    , m_pCurrentPass(nullptr)
    , m_pCurrentFilter(nullptr)
    , m_nNumLightInstances(254)
{
}

CGEKRenderSystem::~CGEKRenderSystem(void)
{
}

STDMETHODIMP CGEKRenderSystem::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKRenderSystem, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
        m_pVideoSystem = GetContext()->GetCachedClass<IGEK3DVideoSystem>(CLSID_GEKVideoSystem);
        m_pEngine = GetContext()->GetCachedClass<IGEKEngine>(CLSID_GEKEngine);
        m_pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
        if (m_pSystem == nullptr ||
            m_pVideoSystem == nullptr ||
            m_pEngine == nullptr ||
            m_pSceneManager == nullptr)
        {
            hRetVal = E_FAIL;
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(m_pSceneManager, (IGEKSceneObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        std::vector<GEK3DVIDEO::INPUTELEMENT> aLayout;
        aLayout.push_back(GEK3DVIDEO::INPUTELEMENT(GEK3DVIDEO::DATA::RG_FLOAT, "POSITION", 0));
        aLayout.push_back(GEK3DVIDEO::INPUTELEMENT(GEK3DVIDEO::DATA::RG_FLOAT, "TEXCOORD", 0));
        hRetVal = m_pVideoSystem->LoadVertexProgram(L"%root%\\data\\programs\\vertex\\overlay.hlsl", "MainVertexProgram", aLayout, &m_spVertexProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to LoadVertexProgram failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->LoadPixelProgram(L"%root%\\data\\programs\\pixel\\overlay.hlsl", "MainPixelProgram", &m_spPixelProgram);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to LoadPixelProgram failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        float2 aVertices[8] =
        {
            float2(0.0f, 0.0f), float2(-1.0f, 1.0f),
            float2(1.0f, 0.0f), float2(1.0f, 1.0f),
            float2(1.0f, 1.0f), float2(1.0f, -1.0f),
            float2(0.0f, 1.0f), float2(-1.0f, -1.0f),
        };

        hRetVal = m_pVideoSystem->CreateBuffer((sizeof(float2) * 2), 4, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &m_spVertexBuffer, aVertices);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        UINT16 aIndices[6] =
        {
            0, 1, 2,
            0, 2, 3,
        };

        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT16), 6, GEK3DVIDEO::BUFFER::INDEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &m_spIndexBuffer, aIndices);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float4x4), 1, GEK3DVIDEO::BUFFER::CONSTANT_BUFFER, &m_spOrthoBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
        if (m_spOrthoBuffer)
        {
            float4x4 nOverlayMatrix;
            nOverlayMatrix.SetOrthographic(0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f);
            m_spOrthoBuffer->Update((void *)&nOverlayMatrix);
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(ENGINEBUFFER), 1, GEK3DVIDEO::BUFFER::CONSTANT_BUFFER, &m_spEngineBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(MATERIALBUFFER), 1, GEK3DVIDEO::BUFFER::CONSTANT_BUFFER, &m_spMaterialBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT32) * 4, 1, GEK3DVIDEO::BUFFER::CONSTANT_BUFFER, &m_spLightCountBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(LIGHT), m_nNumLightInstances, GEK3DVIDEO::BUFFER::DYNAMIC | GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER | GEK3DVIDEO::BUFFER::RESOURCE, &m_spLightBuffer);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEK3DVIDEO::SAMPLERSTATES kStates;
        kStates.m_eFilter = GEK3DVIDEO::FILTER::MIN_MAG_MIP_POINT;
        kStates.m_eAddressU = GEK3DVIDEO::ADDRESS::CLAMP;
        kStates.m_eAddressV = GEK3DVIDEO::ADDRESS::CLAMP;
        hRetVal = m_pVideoSystem->CreateSamplerStates(kStates, &m_spPointSampler);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateSamplerStates failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEK3DVIDEO::SAMPLERSTATES kStates;
        if (m_pSystem->GetConfig().DoesValueExists(L"render", L"anisotropy"))
        {
            kStates.m_nMaxAnisotropy = StrToUINT32(m_pSystem->GetConfig().GetValue(L"render", L"anisotropy", L"1"));
            kStates.m_eFilter = GEK3DVIDEO::FILTER::ANISOTROPIC;
        }
        else
        {
            kStates.m_eFilter = GEK3DVIDEO::FILTER::MIN_MAG_MIP_LINEAR;
        }

        kStates.m_eAddressU = GEK3DVIDEO::ADDRESS::WRAP;
        kStates.m_eAddressV = GEK3DVIDEO::ADDRESS::WRAP;
        hRetVal = m_pVideoSystem->CreateSamplerStates(kStates, &m_spLinearSampler);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateSamplerStates failed: 0x%08X", hRetVal);
    }

    if (SUCCEEDED(hRetVal))
    {
        UINT32 nXSize = m_pSystem->GetXSize();
        UINT32 nYSize = m_pSystem->GetYSize();
        hRetVal = m_pVideoSystem->CreateRenderTarget(nXSize, nYSize, GEK3DVIDEO::DATA::RGBA_UINT8, &m_spScreenBuffer);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEK3DVIDEO::RENDERSTATES kRenderStates;
        hRetVal = m_pVideoSystem->CreateRenderStates(kRenderStates, &m_spRenderStates);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEK3DVIDEO::UNIFIEDBLENDSTATES kBlendStates;
        hRetVal = m_pVideoSystem->CreateBlendStates(kBlendStates, &m_spBlendStates);
    }
   
    if (SUCCEEDED(hRetVal))
    {
        GEK3DVIDEO::DEPTHSTATES kDepthStates;
        hRetVal = m_pVideoSystem->CreateDepthStates(kDepthStates, &m_spDepthStates);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateEvent(&m_spFrameEvent);
        GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateEvent failed: 0x%08X", hRetVal);
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::Destroy(void)
{
    m_pCurrentPass = nullptr;
    m_pCurrentFilter = nullptr;
    m_aVisibleLights.clear();
    m_aResources.clear();
    m_aFilters.clear();
    m_aPasses.clear();

    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationSystem, (IGEKSceneObserver *)GetUnknown());
    GetContext()->RemoveCachedClass(CLSID_GEKRenderSystem);
}

STDMETHODIMP_(void) CGEKRenderSystem::OnLoadBegin(void)
{
    m_aResources.clear();
    m_aFilters.clear();
    m_aPasses.clear();
}

STDMETHODIMP CGEKRenderSystem::OnLoadEnd(HRESULT hRetVal)
{
    if (SUCCEEDED(hRetVal))
    {
        m_pVideoSystem->SetEvent(m_spFrameEvent);
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::OnFree(void)
{
    m_aResources.clear();
    m_aFilters.clear();
    m_aPasses.clear();
    m_pCurrentPass = nullptr;
    m_pCurrentFilter = nullptr;
    m_aVisibleLights.clear();
}

HRESULT CGEKRenderSystem::LoadPass(LPCWSTR pName)
{
    HRESULT hRetVal = E_FAIL;
    auto pPassIterator = m_aPasses.find(pName);
    if (pPassIterator != m_aPasses.end())
    {
        hRetVal = S_OK;
    }
    else
    {
        GEKFUNCTION(L"Name(%s)", pName);

        CLibXMLDoc kDocument;
        CStringW strFileName(FormatString(L"%%root%%\\data\\passes\\%s.xml", pName));
        hRetVal = kDocument.Load(strFileName);
        if (SUCCEEDED(hRetVal))
        {
            PASS kPassData;
            hRetVal = E_INVALID;
            CLibXMLNode kPassNode = kDocument.GetRoot();
            if (kPassNode)
            {
                hRetVal = E_INVALID;
                CLibXMLNode kFiltersNode = kPassNode.FirstChildElement(L"filters");
                if (kFiltersNode)
                {
                    CLibXMLNode kFilterNode = kFiltersNode.FirstChildElement(L"filter");
                    while (kFilterNode)
                    {
                        CStringW strFilter = kFilterNode.GetAttribute(L"source");
                        auto pFilterIterator = m_aFilters.find(strFilter);
                        if (pFilterIterator != m_aFilters.end())
                        {
                            kPassData.m_aFilters.push_back((*pFilterIterator).second);
                            hRetVal = S_OK;
                        }
                        else
                        {
                            CComPtr<IGEKRenderFilter> spFilter;
                            hRetVal = GetContext()->CreateInstance(CLSID_GEKRenderFilter, IID_PPV_ARGS(&spFilter));
                            if (spFilter)
                            {
                                CStringW strFilterFileName(L"%root%\\data\\filters\\" + strFilter + L".xml");
                                hRetVal = spFilter->Load(strFilterFileName);
                                if (SUCCEEDED(hRetVal))
                                {
                                    m_aFilters[strFilter] = spFilter;
                                    kPassData.m_aFilters.push_back(spFilter);
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }

                        kFilterNode = kFilterNode.NextSiblingElement(L"filter");
                    };
                }
            }

            if (SUCCEEDED(hRetVal))
            {
                m_aPasses[pName] = kPassData;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderSystem::LoadResource(LPCWSTR pName, IUnknown **ppResource)
{
    REQUIRE_RETURN(pName, E_INVALIDARG);
    REQUIRE_RETURN(ppResource, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppResource));
    }
    else
    {
        GEKFUNCTION(L"Name(%s)", pName);
        CComPtr<IUnknown> spTexture;
        if (pName[0] == L'*')
        {
            int nPosition = 0;
            CStringW strName = &pName[1];
            CStringW strType = strName.Tokenize(L":", nPosition);
            if (strType.CompareNoCase(L"color") == 0)
            {
                CStringW strColor = strName.Tokenize(L":", nPosition);
                GEKLOG(L"Creating Color Texture: %s", strColor.GetString());
                float4 nColor = StrToFloat4(strColor);

                CComPtr<IGEK3DVideoTexture> spColorTexture;
                hRetVal = m_pVideoSystem->CreateTexture(1, 1, 1, GEK3DVIDEO::DATA::RGBA_UINT8, GEK3DVIDEO::TEXTURE::RESOURCE, &spColorTexture);
                if (spColorTexture)
                {
                    UINT32 nColorValue = UINT32(UINT8(nColor.r * 255.0f)) |
                        UINT32(UINT8(nColor.g * 255.0f) << 8) |
                        UINT32(UINT8(nColor.b * 255.0f) << 16) |
                        UINT32(UINT8(nColor.a * 255.0f) << 24);
                    m_pVideoSystem->UpdateTexture(spColorTexture, &nColorValue, 4);
                    spTexture = spColorTexture;
                }
            }
        }
        else
        {
            GEKLOG(L"Loading Texture: %s", pName);
            CComPtr<IGEK3DVideoTexture> spFileTexture;
            hRetVal = m_pVideoSystem->LoadTexture(FormatString(L"%%root%%\\data\\textures\\%s", pName), &spFileTexture);
            if (spFileTexture)
            {
                spTexture = spFileTexture;
            }
        }

        if (spTexture)
        {
            spTexture->QueryInterface(IID_PPV_ARGS(&m_aResources[pName]));
            hRetVal = spTexture->QueryInterface(IID_PPV_ARGS(ppResource));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::SetResource(IGEK3DVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pResource)
{
    REQUIRE_VOID_RETURN(pSystem);
    CComPtr<IUnknown> spResource(pResource);
    if (spResource)
    {
        pSystem->SetResource(nStage, spResource);
    }
}

STDMETHODIMP CGEKRenderSystem::GetBuffer(LPCWSTR pName, IUnknown **ppResource)
{
    REQUIRE_RETURN(ppResource, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;

    int nPosition = 0;
    CStringW strName = pName;
    CStringW strFilter = strName.Tokenize(L".", nPosition);
    CStringW strSource = strName.Tokenize(L".", nPosition);
    for (auto &kPair : m_aFilters)
    {
        if (kPair.first == strFilter)
        {
            hRetVal = kPair.second->GetBuffer(strSource, ppResource);
            break;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderSystem::GetDepthBuffer(LPCWSTR pSource, IUnknown **ppBuffer)
{
    HRESULT hRetVal = E_FAIL;
    for (auto &kPair : m_aFilters)
    {
        if (kPair.first == pSource)
        {
            hRetVal = kPair.second->GetDepthBuffer(ppBuffer);
            break;
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::SetScreenTargets(IGEK3DVideoContext *pContext, IUnknown *pDepthBuffer)
{
    REQUIRE_VOID_RETURN(m_pVideoSystem);

    pContext->SetRenderTargets({ m_spScreenBuffer }, (pDepthBuffer ? pDepthBuffer : nullptr));
    pContext->SetViewports({ m_kScreenViewPort });
}

STDMETHODIMP CGEKRenderSystem::LoadMaterial(LPCWSTR pName, IUnknown **ppMaterial)
{
    REQUIRE_RETURN(ppMaterial, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppMaterial));
    }
    else
    {
        GEKFUNCTION(L"Name(%s)", pName);

        CLibXMLDoc kDocument;
        hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\materials\\%s.xml", pName));
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = E_INVALIDARG;
            CLibXMLNode kMaterialNode = kDocument.GetRoot();
            if (kMaterialNode)
            {
                CPathW kName(pName);
                kName.RemoveFileSpec();

                bool bFullBright = false;
                if (kMaterialNode.HasAttribute(L"fullbright"))
                {
                    bFullBright = StrToBoolean(kMaterialNode.GetAttribute(L"fullbright"));
                }

                float4 nColor(1.0f, 1.0f, 1.0f, 1.0f);
                if (kMaterialNode.HasAttribute(L"color"))
                {
                    nColor = StrToFloat4(kMaterialNode.GetAttribute(L"color"));
                }

                CComPtr<IUnknown> spAlbedoMap;
                CLibXMLNode kAlbedoNode = kMaterialNode.FirstChildElement(L"albedo");
                CStringW strAlbedo = kAlbedoNode.GetAttribute(L"source");
                strAlbedo.Replace(L"%material%", pName);
                strAlbedo.Replace(L"%directory%", kName.m_strPath.GetString());
                LoadResource(strAlbedo, &spAlbedoMap);
                if (!spAlbedoMap)
                {
                    LoadResource(L"*color:1,1,1,1", &spAlbedoMap);
                }

                CComPtr<IUnknown> spNormalMap;
                CLibXMLNode kNormalNode = kMaterialNode.FirstChildElement(L"normal");
                CStringW strNormal = kNormalNode.GetAttribute(L"source");
                strNormal.Replace(L"%material%", pName);
                strNormal.Replace(L"%directory%", kName.m_strPath.GetString());
                LoadResource(strNormal, &spNormalMap);
                if (!spNormalMap)
                {
                    LoadResource(L"*color:0.5,0.5,1,1", &spNormalMap);
                }

                CComPtr<IUnknown> spInfoMap;
                CLibXMLNode kInfoNode = kMaterialNode.FirstChildElement(L"info");
                CStringW strInfo = kInfoNode.GetAttribute(L"source");
                strInfo.Replace(L"%material%", pName);
                strInfo.Replace(L"%directory%", kName.m_strPath.GetString());
                LoadResource(strInfo, &spInfoMap);
                if (!spInfoMap)
                {
                    LoadResource(L"*color:0.5,0,0,0", &spInfoMap);
                }

                CComPtr<CGEKMaterial> spMaterial(new CGEKMaterial(spAlbedoMap, spNormalMap, spInfoMap, nColor, bFullBright));
                GEKRESULT(spMaterial, L"Unable to allocate new material instance");
                if (spMaterial)
                {
                    spMaterial->CGEKRenderStates::Load(m_pVideoSystem, kMaterialNode.FirstChildElement(L"render"));
                    spMaterial->CGEKBlendStates::Load(m_pVideoSystem, kMaterialNode.FirstChildElement(L"blend"));
                    spMaterial->QueryInterface(IID_PPV_ARGS(&m_aResources[pName]));
                    hRetVal = spMaterial->QueryInterface(IID_PPV_ARGS(ppMaterial));
                }
            }
        }
    }

    if (!(*ppMaterial))
    {
        HRESULT hRetVal = E_FAIL;
        auto pIterator = m_aResources.find(L"*default");
        if (pIterator != m_aResources.end())
        {
            hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppMaterial));
        }
        else
        {
            CComPtr<IUnknown> spAlbedoMap;
            LoadResource(L"*color:1,1,1,1", &spAlbedoMap);

            CComPtr<IUnknown> spNormalMap;
            LoadResource(L"*color:0.5,0.5,1,1", &spNormalMap);

            CComPtr<IUnknown> spInfoMap;
            LoadResource(L"*color:0.5,0,0,0", &spInfoMap);

            CComPtr<CGEKMaterial> spMaterial(new CGEKMaterial(spAlbedoMap, spNormalMap, spInfoMap, float4(1.0f, 1.0f, 1.0f, 1.0f), false));
            GEKRESULT(spMaterial, L"Unable to allocate material new instance");
            if (spMaterial)
            {
                CLibXMLNode kBlankNode(nullptr);
                spMaterial->CGEKRenderStates::Load(m_pVideoSystem, kBlankNode);
                spMaterial->CGEKBlendStates::Load(m_pVideoSystem, kBlankNode);
                spMaterial->QueryInterface(IID_PPV_ARGS(&m_aResources[L"*default"]));
                hRetVal = spMaterial->QueryInterface(IID_PPV_ARGS(ppMaterial));
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(bool) CGEKRenderSystem::EnableMaterial(IGEK3DVideoContext *pContext, IUnknown *pMaterial)
{
    REQUIRE_RETURN(pContext && pMaterial, false);

    bool bReturn = false;
    CComQIPtr<IGEKMaterial> spMaterial(pMaterial);
    if (spMaterial)
    {
        bReturn = true;
        MATERIALBUFFER kMaterial;
        kMaterial.m_nColor = spMaterial->GetColor();
        kMaterial.m_bFullBright = spMaterial->IsFullBright();
        m_spMaterialBuffer->Update((void *)&kMaterial);
        spMaterial->Enable(this, pContext);
    }

    return bReturn;
}

STDMETHODIMP CGEKRenderSystem::LoadProgram(LPCWSTR pName, IUnknown **ppProgram)
{
    REQUIRE_RETURN(ppProgram, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppProgram));
    }
    else
    {
        GEKFUNCTION(L"Name(%s)", pName);

        CStringA strDeferredProgram;
        hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\vertex\\plugin.hlsl", strDeferredProgram);
        if (SUCCEEDED(hRetVal))
        {
            if (strDeferredProgram.Find("_INSERT_WORLD_PROGRAM") < 0)
            {
                hRetVal = E_INVALID;
            }
            else
            {
                CLibXMLDoc kDocument;
                hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\programs\\vertex\\%s.xml", pName));
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = E_INVALIDARG;
                    CLibXMLNode kProgramNode = kDocument.GetRoot();
                    if (kProgramNode)
                    {
                        CLibXMLNode kLayoutNode = kProgramNode.FirstChildElement(L"layout");
                        if (kLayoutNode)
                        {
                            std::vector<CStringA> aNames;
                            std::vector<GEK3DVIDEO::INPUTELEMENT> aLayout;
                            CLibXMLNode kElementNode = kLayoutNode.FirstChildElement(L"element");
                            while (kElementNode)
                            {
                                if (kElementNode.HasAttribute(L"type") && 
                                   kElementNode.HasAttribute(L"name") &&
                                   kElementNode.HasAttribute(L"index"))
                                {
                                    aNames.push_back((LPCSTR)CW2A(kElementNode.GetAttribute(L"name")));

                                    GEK3DVIDEO::INPUTELEMENT kData;
                                    kData.m_eType = GetFormatType(kElementNode.GetAttribute(L"type"));
                                    kData.m_pName = aNames.back().GetString();
                                    kData.m_nIndex = StrToUINT32(kElementNode.GetAttribute(L"index"));
                                    if (kElementNode.HasAttribute(L"class") &&
                                       kElementNode.HasAttribute(L"slot"))
                                    {
                                        kData.m_eClass = GetElementClass(kElementNode.GetAttribute(L"class"));
                                        kData.m_nSlot = StrToUINT32(kElementNode.GetAttribute(L"slot"));
                                    }

                                    aLayout.push_back(kData);
                                }
                                else
                                {
                                    break;
                                }

                                kElementNode = kElementNode.NextSiblingElement(L"element");
                            };

                            hRetVal = S_OK;
                            CComPtr<IUnknown> spGeometryProgram;
                            CLibXMLNode kGeometryNode = kProgramNode.FirstChildElement(L"geometry");
                            if (kGeometryNode)
                            {
                                CStringA strGeometryProgram = kGeometryNode.GetText();
                                hRetVal = m_pVideoSystem->CompileGeometryProgram(strGeometryProgram, "MainGeometryProgram", &spGeometryProgram);
                            }

                            if (SUCCEEDED(hRetVal))
                            {
                                hRetVal = E_INVALIDARG;
                                CLibXMLNode kVertexNode = kProgramNode.FirstChildElement(L"vertex");
                                if (kVertexNode)
                                {
                                    CStringA strVertexProgram = kVertexNode.GetText();
                                    strDeferredProgram.Replace("_INSERT_WORLD_PROGRAM", (strVertexProgram + "\r\n"));

                                    CComPtr<IUnknown> spVertexProgram;
                                    hRetVal = m_pVideoSystem->CompileVertexProgram(strDeferredProgram, "MainVertexProgram", aLayout, &spVertexProgram);
                                    if (spVertexProgram)
                                    {
                                        CComPtr<CGEKProgram> spProgram(new CGEKProgram(spVertexProgram, spGeometryProgram));
                                        GEKRESULT(spProgram, L"Unable to allocate new program instance");
                                        if (spProgram)
                                        {
                                            spProgram->QueryInterface(IID_PPV_ARGS(&m_aResources[pName]));
                                            hRetVal = spProgram->QueryInterface(IID_PPV_ARGS(ppProgram));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::EnableProgram(IGEK3DVideoContext *pContext, IUnknown *pProgram)
{
    REQUIRE_VOID_RETURN(m_pVideoSystem);
    REQUIRE_VOID_RETURN(pProgram);

    CComQIPtr<IGEKProgram> spProgram(pProgram);
    if (spProgram)
    {
        pContext->GetVertexSystem()->SetProgram(spProgram->GetVertexProgram());
        pContext->GetGeometrySystem()->SetProgram(spProgram->GetGeometryProgram());
    }
}

STDMETHODIMP_(void) CGEKRenderSystem::DrawScene(IGEK3DVideoContext *pContext, UINT32 nAttributes)
{
    CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnDrawScene, std::placeholders::_1, pContext, nAttributes)));
}

STDMETHODIMP_(void) CGEKRenderSystem::DrawLights(IGEK3DVideoContext *pContext, std::function<void(void)> OnLightBatch)
{
    pContext->GetVertexSystem()->SetProgram(m_spVertexProgram);
    pContext->GetGeometrySystem()->SetProgram(nullptr);
    pContext->GetPixelSystem()->SetResource(0, m_spLightBuffer);
    pContext->GetComputeSystem()->SetResource(0, m_spLightBuffer);

    pContext->SetVertexBuffer(0, 0, m_spVertexBuffer);
    pContext->SetIndexBuffer(0, m_spIndexBuffer);
    pContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);

    for (UINT32 nPass = 0; nPass < m_aVisibleLights.size(); nPass += m_nNumLightInstances)
    {
        UINT32 nNumLights = min(m_nNumLightInstances, (m_aVisibleLights.size() - nPass));
        if (nNumLights > 1)
        {
            UINT32 aCounts[4] =
            {
                nNumLights, 0, 0, 0,
            };

            m_spLightCountBuffer->Update(aCounts);

            LIGHT *pLights = nullptr;
            if (SUCCEEDED(m_spLightBuffer->Map((LPVOID *)&pLights)))
            {
                memcpy(pLights, &m_aVisibleLights[nPass], (sizeof(LIGHT)* nNumLights));
                m_spLightBuffer->UnMap();

                OnLightBatch();

                pContext->DrawIndexedPrimitive(6, 0, 0);
            }
        }
    }
}

STDMETHODIMP_(void) CGEKRenderSystem::DrawOverlay(IGEK3DVideoContext *pContext)
{
    pContext->GetVertexSystem()->SetProgram(m_spVertexProgram);

    pContext->GetGeometrySystem()->SetProgram(nullptr);

    pContext->SetVertexBuffer(0, 0, m_spVertexBuffer);
    pContext->SetIndexBuffer(0, m_spIndexBuffer);
    pContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);

    pContext->DrawIndexedPrimitive(6, 0, 0);
}

STDMETHODIMP_(void) CGEKRenderSystem::Render(void)
{
    REQUIRE_VOID_RETURN(m_pSceneManager && m_pVideoSystem);

    CComQIPtr<IGEK3DVideoContext> spContext(m_pVideoSystem);
    spContext->GetPixelSystem()->SetSamplerStates(0, m_spPointSampler);
    spContext->GetPixelSystem()->SetSamplerStates(1, m_spLinearSampler);

    m_pSceneManager->ListComponentsEntities({ L"transform", L"viewer" }, [&](const GEKENTITYID &nViewerID)->void
    {
        GEKSETMETRIC("NUMLIGHTS", 0);
        GEKSETMETRIC("NUMOBJECTS", 0);

        GEKVALUE kPass;
        m_pSceneManager->GetProperty(nViewerID, L"viewer", L"pass", kPass);
        if (SUCCEEDED(LoadPass(kPass.GetRawString())))
        {
            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnPreRender, std::placeholders::_1)));

            GEKVALUE kProjection;
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"projection", kProjection);

            GEKVALUE kMinViewDistance;
            GEKVALUE kMaxViewDistance;
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"minviewdistance", kMinViewDistance);
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"maxviewdistance", kMaxViewDistance);

            GEKVALUE kFieldOfView;
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"fieldofview", kFieldOfView);
            float nFieldOfView = _DEGTORAD(kFieldOfView.GetFloat());

            GEKVALUE kViewPort;
            m_pSceneManager->GetProperty(nViewerID, L"viewer", L"viewport", kViewPort);
            m_kScreenViewPort.m_nTopLeftX = kViewPort.GetFloat4().x * m_pSystem->GetXSize();
            m_kScreenViewPort.m_nTopLeftY = kViewPort.GetFloat4().y * m_pSystem->GetYSize();
            m_kScreenViewPort.m_nXSize = kViewPort.GetFloat4().z * m_pSystem->GetXSize();
            m_kScreenViewPort.m_nYSize = kViewPort.GetFloat4().w * m_pSystem->GetYSize();
            m_kScreenViewPort.m_nMinDepth = 0.0f;
            m_kScreenViewPort.m_nMaxDepth = 1.0f;

            GEKVALUE kPosition;
            GEKVALUE kRotation;
            m_pSceneManager->GetProperty(nViewerID, L"transform", L"position", kPosition);
            m_pSceneManager->GetProperty(nViewerID, L"transform", L"rotation", kRotation);

            float4x4 nCameraMatrix;
            nCameraMatrix = kRotation.GetQuaternion();
            nCameraMatrix.t = kPosition.GetFloat3();

            float nXSize = float(m_pSystem->GetXSize());
            float nYSize = float(m_pSystem->GetYSize());
            float nAspect = (nXSize / nYSize);

            m_kCurrentBuffer.m_nCameraSize.x = nXSize;
            m_kCurrentBuffer.m_nCameraSize.y = nYSize;
            m_kCurrentBuffer.m_nCameraView.x = tan(nFieldOfView * 0.5f);
            m_kCurrentBuffer.m_nCameraView.y = (m_kCurrentBuffer.m_nCameraView.x / nAspect);
            m_kCurrentBuffer.m_nCameraViewDistance = kMaxViewDistance.GetFloat();
            m_kCurrentBuffer.m_nCameraPosition = kPosition.GetFloat3();

            m_kCurrentBuffer.m_nViewMatrix = nCameraMatrix.GetInverse();
            m_kCurrentBuffer.m_nProjectionMatrix = kProjection.GetFloat4x4();
            m_kCurrentBuffer.m_nTransformMatrix = (m_kCurrentBuffer.m_nViewMatrix * m_kCurrentBuffer.m_nProjectionMatrix);

            m_nCurrentFrustum.Create(nCameraMatrix, m_kCurrentBuffer.m_nProjectionMatrix);

            LIGHT kLight;
            m_aVisibleLights.clear();
            m_pSceneManager->ListComponentsEntities({ L"transform", L"light" }, [&](const GEKENTITYID &nEntityID)->void
            {
                GEKVALUE kValue;
                m_pSceneManager->GetProperty(nEntityID, L"transform", L"position", kValue);
                kLight.m_nPosition = (m_kCurrentBuffer.m_nViewMatrix * float4(kValue.GetFloat3(), 1.0f));

                m_pSceneManager->GetProperty(nEntityID, L"light", L"range", kValue);
                kLight.m_nInvRange = (1.0f / (kLight.m_nRange = kValue.GetFloat()));

                m_pSceneManager->GetProperty(nEntityID, L"light", L"color", kValue);
                kLight.m_nColor = kValue.GetFloat3();

                m_aVisibleLights.push_back(kLight);
            });

            GEKSETMETRIC("NUMLIGHTS", m_aVisibleLights.size());
            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnCullScene, std::placeholders::_1)));

            m_spEngineBuffer->Update((void *)&m_kCurrentBuffer);
            spContext->GetGeometrySystem()->SetConstantBuffer(0, m_spEngineBuffer);

            spContext->GetVertexSystem()->SetConstantBuffer(0, m_spEngineBuffer);
            spContext->GetVertexSystem()->SetConstantBuffer(1, m_spOrthoBuffer);

            spContext->GetComputeSystem()->SetConstantBuffer(0, m_spEngineBuffer);
            spContext->GetComputeSystem()->SetConstantBuffer(1, m_spLightCountBuffer);

            spContext->GetPixelSystem()->SetConstantBuffer(0, m_spEngineBuffer);
            spContext->GetPixelSystem()->SetConstantBuffer(1, m_spMaterialBuffer);
            spContext->GetPixelSystem()->SetConstantBuffer(2, m_spLightCountBuffer);

            m_pCurrentPass = &m_aPasses[kPass.GetRawString()];
            for (auto &pFilter : m_pCurrentPass->m_aFilters)
            {
                m_pCurrentFilter = pFilter;
                pFilter->Draw(spContext);
                m_pCurrentFilter = nullptr;
            }

            m_pCurrentPass = nullptr;
            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnPostRender, std::placeholders::_1)));
        }

        GEKLOGMETRICS(L"Viewer: %d", nViewerID);
    });

    m_pVideoSystem->SetDefaultTargets(spContext);
    spContext->SetRenderStates(m_spRenderStates);
    spContext->SetBlendStates(float4(1.0f), 0xFFFFFFFF, m_spBlendStates);
    spContext->SetDepthStates(0x0, m_spDepthStates);
    spContext->GetComputeSystem()->SetProgram(nullptr);
    spContext->GetVertexSystem()->SetProgram(m_spVertexProgram);
    spContext->GetVertexSystem()->SetConstantBuffer(1, m_spOrthoBuffer);
    spContext->GetGeometrySystem()->SetProgram(nullptr);
    spContext->GetPixelSystem()->SetProgram(m_spPixelProgram);
    spContext->GetPixelSystem()->SetResource(0, m_spScreenBuffer);
    SetResource(spContext->GetPixelSystem(), 1, nullptr);
    spContext->SetVertexBuffer(0, 0, m_spVertexBuffer);
    spContext->SetIndexBuffer(0, m_spIndexBuffer);
    spContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);
    spContext->DrawIndexedPrimitive(6, 0, 0);

    spContext->ClearResources();

    CComQIPtr<IGEK2DVideoSystem> sp2DVideoSystem(m_pVideoSystem);
    if (sp2DVideoSystem)
    {
        sp2DVideoSystem->Begin();

        CComPtr<IUnknown> spRed;
        sp2DVideoSystem->CreateBrush(float4(1.0f, 0.0f, 0.0f, 1.0f), &spRed);

        CComPtr<IUnknown> spGray;
        sp2DVideoSystem->CreateBrush(float4(0.5f, 0.25f, 0.25f, 1.0f), &spGray);

        CComPtr<IUnknown> spGradient;
        sp2DVideoSystem->CreateBrush({ { 0.0f, float4(0.0f, 1.0f, 0.0f, 1.0f) }, { 1.0f, float4(0.0f, 0.0f, 1.0f, 1.0f) } }, { 0.0f, 0.0f, 1000.0f, 1000.0f }, &spGradient);

        CComPtr<IUnknown> spGradient1;
        sp2DVideoSystem->CreateBrush({ { 0.0f, float4(0.5f, 0.0f, 0.0f, 1.0f) }, { 1.0f, float4(1.0f, 0.0f, 0.0f, 1.0f) } }, { 0.0f, 0.0f, 250.0f, 0.0f }, &spGradient1);
        CComPtr<IUnknown> spGradient2;
        sp2DVideoSystem->CreateBrush({ { 0.0f, float4(1.0f, 0.0f, 0.0f, 1.0f) }, { 1.0f, float4(0.5f, 0.0f, 0.0f, 1.0f) } }, { 0.0f, 0.0f, 250.0f, 0.0f }, &spGradient2);

        CComPtr<IUnknown> spFont;
        sp2DVideoSystem->CreateFont(L"Arial", 400, GEK2DVIDEO::FONT::NORMAL, 25.0f, &spFont);

        float3x2 nTransform;
        sp2DVideoSystem->SetTransform(nTransform);
        sp2DVideoSystem->DrawRectangle({ 10.0f, 10.0f, 240.0f, 65.f }, float2(5.0f, 5.0f), spGradient1, true);
        sp2DVideoSystem->DrawRectangle({ 15.0f, 15.0f, 235.0f, 60.f }, float2(5.0f, 5.0f), spGradient2, true);
        sp2DVideoSystem->DrawText({ 25.0f, 25.0f, 225.0f, 50.0f }, spFont, spGray, L"Test: %d", 1);

        CComPtr<IGEK2DVideoGeometry> spGeometry;
        sp2DVideoSystem->CreateGeometry(&spGeometry);
        spGeometry->Open();
        spGeometry->Begin(float2(0.0f, 0.0f), true);
        spGeometry->AddLine(float2(100.0f, 0.0f));
        spGeometry->AddLine(float2(50.0f, 50.0f));
        spGeometry->End(false);
        spGeometry->Close();
        
        CComPtr<IGEK2DVideoGeometry> spWideGeometry;
        spGeometry->Widen(5.0f, 0.0f, &spWideGeometry);

        nTransform.SetTranslation(float2(500.0f, 500.0f));
        sp2DVideoSystem->SetTransform(nTransform);
        sp2DVideoSystem->DrawGeometry(spGeometry, spGradient, true);
        sp2DVideoSystem->DrawGeometry(spWideGeometry, spGray, true);

        nTransform.SetTranslation(float2(700.0f, 500.0f));
        sp2DVideoSystem->SetTransform(nTransform);
        sp2DVideoSystem->DrawGeometry(spGeometry, spGradient, true);
        sp2DVideoSystem->DrawGeometry(spWideGeometry, spGray, true);

        nTransform.SetTranslation(float2(600.0f, 600.0f));
        sp2DVideoSystem->SetTransform(nTransform);
        sp2DVideoSystem->DrawGeometry(spGeometry, spGradient, true);
        sp2DVideoSystem->DrawGeometry(spWideGeometry, spGray, true);

        sp2DVideoSystem->End();
    }

    m_pVideoSystem->Present(true);

    while (!m_pVideoSystem->IsEventSet(m_spFrameEvent))
    {
        Sleep(0);
    };

    m_pVideoSystem->SetEvent(m_spFrameEvent);
}

STDMETHODIMP_(float2) CGEKRenderSystem::GetScreenSize(void) const
{
    REQUIRE_RETURN(m_spScreenBuffer, 0.0f);
    return float2(float(m_spScreenBuffer->GetXSize()), float(m_spScreenBuffer->GetYSize()));
}

STDMETHODIMP_(const frustum &) CGEKRenderSystem::GetFrustum(void) const
{
    return m_nCurrentFrustum;
}
