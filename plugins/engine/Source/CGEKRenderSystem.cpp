#include "CGEKRenderSystem.h"
#include "IGEKRenderFilter.h"
#include "CGEKProperties.h"
#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"
#include <windowsx.h>
#include <atlpath.h>

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
    INTERFACE_LIST_ENTRY_COM(IGEK3DVideoObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKProgramManager)
    INTERFACE_LIST_ENTRY_COM(IGEKMaterialManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderSystem)

CGEKRenderSystem::CGEKRenderSystem(void)
    : m_pVideoSystem(nullptr)
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
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKRenderSystem, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        m_pVideoSystem = GetContext()->GetCachedClass<IGEK3DVideoSystem>(CLSID_GEKVideoSystem);
        m_pEngine = GetContext()->GetCachedClass<IGEKEngine>(CLSID_GEKEngine);
        m_pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
        if (m_pVideoSystem == nullptr ||
            m_pEngine == nullptr ||
            m_pSceneManager == nullptr)
        {
            hRetVal = E_FAIL;
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(m_pVideoSystem, (IGEK3DVideoObserver *)GetUnknown());
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
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->LoadPixelProgram(L"%root%\\data\\programs\\pixel\\overlay.hlsl", "MainPixelProgram", &m_spPixelProgram);
    }

    if (SUCCEEDED(hRetVal))
    {
        float2 aVertices[] =
        {
            float2(0.0f, 0.0f),
            float2(1.0f, 0.0f),
            float2(1.0f, 1.0f),
            float2(0.0f, 1.0f),
        };

        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float2), 4, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &m_spVertexBuffer, aVertices);
    }

    if (SUCCEEDED(hRetVal))
    {
        UINT16 aIndices[6] =
        {
            0, 1, 2,
            0, 2, 3,
        };

        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT16), 6, GEK3DVIDEO::BUFFER::INDEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, &m_spIndexBuffer, aIndices);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(float4x4), 1, GEK3DVIDEO::BUFFER::CONSTANT_BUFFER, &m_spOrthoBuffer);
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
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(MATERIALBUFFER), 1, GEK3DVIDEO::BUFFER::CONSTANT_BUFFER, &m_spMaterialBuffer);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(UINT32) * 4, 1, GEK3DVIDEO::BUFFER::CONSTANT_BUFFER, &m_spLightCountBuffer);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_pVideoSystem->CreateBuffer(sizeof(LIGHTBUFFER), m_nNumLightInstances, GEK3DVIDEO::BUFFER::DYNAMIC | GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER | GEK3DVIDEO::BUFFER::RESOURCE, &m_spLightBuffer);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEK3DVIDEO::SAMPLERSTATES kStates;
        kStates.m_eFilter = GEK3DVIDEO::FILTER::MIN_MAG_MIP_POINT;
        kStates.m_eAddressU = GEK3DVIDEO::ADDRESS::CLAMP;
        kStates.m_eAddressV = GEK3DVIDEO::ADDRESS::CLAMP;
        hRetVal = m_pVideoSystem->CreateSamplerStates(kStates, &m_spPointSampler);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEK3DVIDEO::SAMPLERSTATES kStates;
        kStates.m_eFilter = GEK3DVIDEO::FILTER::MIN_MAG_MIP_LINEAR;

        CLibXMLDoc kDocument;
        if (SUCCEEDED(kDocument.Load(L"%root%\\config.xml")))
        {
            CLibXMLNode kRoot = kDocument.GetRoot();
            if (kRoot && kRoot.GetType().CompareNoCase(L"config") == 0 && kRoot.HasChildElement(L"render"))
            {
                CLibXMLNode kRender = kRoot.FirstChildElement(L"render");
                if (kRender)
                {
                    if (kRender.HasAttribute(L"anisotropy"))
                    {
                        kStates.m_nMaxAnisotropy = StrToUINT32(kRender.GetAttribute(L"anisotropy"));
                        kStates.m_eFilter = GEK3DVIDEO::FILTER::ANISOTROPIC;
                    }
                }
            }
        }

        kStates.m_eAddressU = GEK3DVIDEO::ADDRESS::WRAP;
        kStates.m_eAddressV = GEK3DVIDEO::ADDRESS::WRAP;
        hRetVal = m_pVideoSystem->CreateSamplerStates(kStates, &m_spLinearWrapSampler);
    }

    if (SUCCEEDED(hRetVal))
    {
        GEK3DVIDEO::SAMPLERSTATES kStates;
        /*if (m_pSystem->GetConfig().DoesValueExists(L"render", L"anisotropy"))
        {
            kStates.m_nMaxAnisotropy = StrToUINT32(m_pSystem->GetConfig().GetValue(L"render", L"anisotropy", L"1"));
            kStates.m_eFilter = GEK3DVIDEO::FILTER::ANISOTROPIC;
        }
        else*/
        {
            kStates.m_eFilter = GEK3DVIDEO::FILTER::MIN_MAG_MIP_LINEAR;
        }

        kStates.m_eAddressU = GEK3DVIDEO::ADDRESS::CLAMP;
        kStates.m_eAddressV = GEK3DVIDEO::ADDRESS::CLAMP;
        hRetVal = m_pVideoSystem->CreateSamplerStates(kStates, &m_spLinearClampSampler);
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
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::Destroy(void)
{
    m_pCurrentPass = nullptr;
    m_pCurrentFilter = nullptr;
    m_aVisibleLights.clear();
    m_aResources.clear();
    m_aBuffers.clear();
    m_aPasses.clear();

    GetContext()->RemoveCachedObserver(CLSID_GEKVideoSystem, (IGEK3DVideoObserver *)GetUnknown());
    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationSystem, (IGEKSceneObserver *)GetUnknown());
    GetContext()->RemoveCachedClass(CLSID_GEKRenderSystem);
}

STDMETHODIMP_(void) CGEKRenderSystem::OnResizeBegin(void)
{
    for (auto &kBuffer : m_aBuffers)
    {
        kBuffer.second.m_spResource.Release();
    }

    for (auto &kPass : m_aPasses)
    {
        kPass.second.m_aBuffers[0].Release();
        kPass.second.m_aBuffers[1].Release();
    }
}

STDMETHODIMP CGEKRenderSystem::OnResizeEnd(UINT32 nXSize, UINT32 nYSize, bool bWindowed)
{
    HRESULT hRetVal = S_OK;
    for (auto &kBuffer : m_aBuffers)
    {
        if (kBuffer.second.m_nStride > 0)
        {
            CComPtr<IGEK3DVideoBuffer> spBuffer;
            hRetVal = m_pVideoSystem->CreateBuffer(kBuffer.second.m_nStride, kBuffer.second.m_nCount, GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER | GEK3DVIDEO::BUFFER::RESOURCE, &spBuffer);
            if (spBuffer)
            {
                hRetVal = spBuffer->QueryInterface(IID_PPV_ARGS(&kBuffer.second.m_spResource));
            }
        }
        else if (kBuffer.second.m_nCount > 0)
        {
            CComPtr<IGEK3DVideoBuffer> spBuffer;
            hRetVal = m_pVideoSystem->CreateBuffer(kBuffer.second.m_eFormat, kBuffer.second.m_nCount, GEK3DVIDEO::BUFFER::UNORDERED_ACCESS | GEK3DVIDEO::BUFFER::RESOURCE, &spBuffer);
            if (spBuffer)
            {
                hRetVal = spBuffer->QueryInterface(IID_PPV_ARGS(&kBuffer.second.m_spResource));
            }
        }
        else if (kBuffer.second.m_eFormat != GEK3DVIDEO::DATA::UNKNOWN)
        {
            switch (kBuffer.second.m_eFormat)
            {
            case GEK3DVIDEO::DATA::D16:
            case GEK3DVIDEO::DATA::D24_S8:
            case GEK3DVIDEO::DATA::D32:
                hRetVal = m_pVideoSystem->CreateDepthTarget(nXSize, nYSize, kBuffer.second.m_eFormat, &kBuffer.second.m_spResource);
                break;

            default:
                if (true)
                {
                    CComPtr<IGEK3DVideoTexture> spTarget;
                    hRetVal = m_pVideoSystem->CreateRenderTarget(nXSize, nYSize, kBuffer.second.m_eFormat, &spTarget);
                    if (spTarget)
                    {
                        hRetVal = spTarget->QueryInterface(IID_PPV_ARGS(&kBuffer.second.m_spResource));
                    }
                }

                break;
            };

            if (FAILED(hRetVal))
            {
                break;
            }
        }
    }

    for (auto &kPass : m_aPasses)
    {
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = m_pVideoSystem->CreateRenderTarget(nXSize, nYSize, GEK3DVIDEO::DATA::RGBA_UINT8, &kPass.second.m_aBuffers[0]);
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = m_pVideoSystem->CreateRenderTarget(nXSize, nYSize, GEK3DVIDEO::DATA::RGBA_UINT8, &kPass.second.m_aBuffers[1]);
        }

        if (FAILED(hRetVal))
        {
            break;
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        for (auto &kPass : m_aPasses)
        {
            for (auto &kFilter : kPass.second.m_aFilters)
            {
                hRetVal = kFilter.m_spFilter->Reload();
                if (FAILED(hRetVal))
                {
                    break;
                }
            }

            if (FAILED(hRetVal))
            {
                break;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::OnLoadBegin(void)
{
    m_aResources.clear();
    m_aBuffers.clear();
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
    m_aBuffers.clear();
    m_aPasses.clear();
    m_pCurrentPass = nullptr;
    m_pCurrentFilter = nullptr;
    m_aVisibleLights.clear();
}

HRESULT CGEKRenderSystem::LoadPass(LPCWSTR pName)
{
    REQUIRE_RETURN(pName, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pPassIterator = m_aPasses.find(pName);
    if (pPassIterator != m_aPasses.end())
    {
        hRetVal = S_OK;
    }
    else
    {

        CLibXMLDoc kDocument;
        CStringW strFileName(FormatString(L"%%root%%\\data\\passes\\%s.xml", pName));
        hRetVal = kDocument.Load(strFileName);
        if (SUCCEEDED(hRetVal))
        {
            PASS kPassData;
            hRetVal = E_FAIL;
            CLibXMLNode kPassNode = kDocument.GetRoot();
            if (kPassNode)
            {
                hRetVal = E_FAIL;
                CLibXMLNode kFiltersNode = kPassNode.FirstChildElement(L"filters");
                if (kFiltersNode)
                {
                    CLibXMLNode kFilterNode = kFiltersNode.FirstChildElement(L"filter");
                    while (kFilterNode)
                    {
                        if (kFilterNode.HasAttribute(L"source") && kFilterNode.HasAttribute(L"material"))
                        {
                            hRetVal = S_OK;
                            std::unordered_map<CStringA, CStringA> aDefines;
                            CLibXMLNode kDefinesNode = kFilterNode.FirstChildElement(L"defines");
                            if (kDefinesNode)
                            {
                                CLibXMLNode kDefineNode = kDefinesNode.FirstChildElement(L"define");
                                while (kDefineNode)
                                {
                                    if (kDefineNode.HasAttribute(L"name") &&
                                        kDefineNode.HasAttribute(L"value"))
                                    {
                                        CStringA strName = CW2A(kDefineNode.GetAttribute(L"name"), CP_UTF8);
                                        CStringA strValue = CW2A(kDefineNode.GetAttribute(L"value"), CP_UTF8);

                                        aDefines[strName] = strValue;
                                        kDefineNode = kDefineNode.NextSiblingElement(L"define");
                                    }
                                    else
                                    {
                                        hRetVal = E_INVALIDARG;
                                        break;
                                    }
                                };
                            }

                            if (SUCCEEDED(hRetVal))
                            {
                                CStringW strFilter(kFilterNode.GetAttribute(L"source"));

                                CComPtr<IGEKRenderFilter> spFilter;
                                hRetVal = GetContext()->CreateInstance(CLSID_GEKRenderFilter, IID_PPV_ARGS(&spFilter));
                                if (spFilter)
                                {
                                    CStringW strFilterFileName(L"%root%\\data\\filters\\" + strFilter + L".xml");
                                    hRetVal = spFilter->Load(strFilterFileName, aDefines);
                                    if (SUCCEEDED(hRetVal))
                                    {
                                        FILTER kFilter;
                                        kFilter.m_strMaterial = kFilterNode.GetAttribute(L"material");
                                        kFilter.m_spFilter = spFilter;
                                        kPassData.m_aFilters.push_back(kFilter);
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }

                                kFilterNode = kFilterNode.NextSiblingElement(L"filter");
                            }
                        }
                        else
                        {
                            hRetVal = E_INVALIDARG;
                            break;
                        }
                    };
                }
            }

            if (SUCCEEDED(hRetVal))
            {
                UINT32 nXSize = m_pSystem->GetXSize();
                UINT32 nYSize = m_pSystem->GetYSize();
                hRetVal = m_pVideoSystem->CreateRenderTarget(nXSize, nYSize, GEK3DVIDEO::DATA::RGBA_UINT8, &kPassData.m_aBuffers[0]);
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_pVideoSystem->CreateRenderTarget(nXSize, nYSize, GEK3DVIDEO::DATA::RGBA_UINT8, &kPassData.m_aBuffers[1]);
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
    REQUIRE_RETURN(pName && ppResource, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppResource));
    }
    else
    {
        CComPtr<IUnknown> spTexture;
        if (pName[0] == L'*')
        {
            int nPosition = 0;
            CStringW strName(&pName[1]);
            CStringW strType(strName.Tokenize(L":", nPosition));
            if (strType.CompareNoCase(L"color") == 0)
            {
                CStringW strColor(strName.Tokenize(L":", nPosition));
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
            CComPtr<IGEK3DVideoTexture> spFileTexture;
            hRetVal = m_pVideoSystem->LoadTexture(FormatString(L"%%root%%\\data\\textures\\%s", pName), 0, &spFileTexture);
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

STDMETHODIMP_(void) CGEKRenderSystem::SetDefaultRenderStates(IUnknown *pStates)
{
    m_spDefaultRenderStates = pStates;
}

STDMETHODIMP_(void) CGEKRenderSystem::SetDefaultBlendStates(const float4 &nBlendFactor, UINT32 nMask, IUnknown *pStates)
{
    m_nDefaultBlendFactor = nBlendFactor;
    m_nDefaultSampleMask = nMask;
    m_spDefaultBlendStates = pStates;
}

STDMETHODIMP_(void) CGEKRenderSystem::EnableDefaultRenderStates(IGEK3DVideoContext *pContext)
{
    REQUIRE_VOID_RETURN(pContext);

    if (m_spDefaultRenderStates)
    {
        pContext->SetRenderStates(m_spDefaultRenderStates);
    }
}

STDMETHODIMP_(void) CGEKRenderSystem::EnableDefaultBlendStates(IGEK3DVideoContext *pContext)
{
    REQUIRE_VOID_RETURN(pContext);

    if (m_spDefaultBlendStates)
    {
        pContext->SetBlendStates(m_nDefaultBlendFactor, m_nDefaultSampleMask, m_spDefaultBlendStates);
    }
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

STDMETHODIMP CGEKRenderSystem::CreateBuffer(LPCWSTR pName, UINT32 nStride, UINT32 nCount)
{
    REQUIRE_RETURN(m_pVideoSystem, E_FAIL);
    REQUIRE_RETURN(pName, E_INVALIDARG);

    HRESULT hRetVal = S_OK;
    if (m_aBuffers.find(pName) == m_aBuffers.end())
    {
        CComPtr<IGEK3DVideoBuffer> spBuffer;
        hRetVal = m_pVideoSystem->CreateBuffer(nStride, nCount, GEK3DVIDEO::BUFFER::STRUCTURED_BUFFER | GEK3DVIDEO::BUFFER::RESOURCE, &spBuffer);
        if (spBuffer)
        {
            BUFFER &kBuffer = m_aBuffers[pName];
            kBuffer.m_nStride = nStride;
            kBuffer.m_nCount = nCount;
            kBuffer.m_spResource = spBuffer;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderSystem::CreateBuffer(LPCWSTR pName, GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nCount)
{
    REQUIRE_RETURN(m_pVideoSystem, E_FAIL);
    REQUIRE_RETURN(pName, E_INVALIDARG);

    HRESULT hRetVal = S_OK;
    if (m_aBuffers.find(pName) == m_aBuffers.end())
    {
        CComPtr<IGEK3DVideoBuffer> spBuffer;
        hRetVal = m_pVideoSystem->CreateBuffer(eFormat, nCount, GEK3DVIDEO::BUFFER::UNORDERED_ACCESS | GEK3DVIDEO::BUFFER::RESOURCE, &spBuffer);
        if (spBuffer)
        {
            BUFFER &kBuffer = m_aBuffers[pName];
            kBuffer.m_eFormat = eFormat;
            kBuffer.m_nCount = nCount;
            kBuffer.m_spResource = spBuffer;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderSystem::CreateBuffer(LPCWSTR pName, UINT32 nXSize, UINT32 nYSize, GEK3DVIDEO::DATA::FORMAT eFormat)
{
    REQUIRE_RETURN(m_pVideoSystem, E_FAIL);
    REQUIRE_RETURN(pName, E_INVALIDARG);

    HRESULT hRetVal = S_OK;
    if (m_aBuffers.find(pName) == m_aBuffers.end())
    {
        CComPtr<IUnknown> spResource;
        switch (eFormat)
        {
        case GEK3DVIDEO::DATA::D16:
        case GEK3DVIDEO::DATA::D24_S8:
        case GEK3DVIDEO::DATA::D32:
            hRetVal = m_pVideoSystem->CreateDepthTarget(nXSize, nYSize, eFormat, &spResource);
            break;

        default:
            if (true)
            {
                CComPtr<IGEK3DVideoTexture> spTarget;
                hRetVal = m_pVideoSystem->CreateRenderTarget(nXSize, nYSize, eFormat, &spTarget);
                if (spTarget)
                {
                    hRetVal = spTarget->QueryInterface(IID_PPV_ARGS(&spResource));
                }
            }

            break;
        };

        if (spResource)
        {
            BUFFER &kBuffer = m_aBuffers[pName];
            kBuffer.m_eFormat = eFormat;
            kBuffer.m_spResource = spResource;
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKRenderSystem::GetBuffer(LPCWSTR pName, IUnknown **ppResource)
{
    REQUIRE_RETURN(m_pCurrentPass, E_FAIL);
    REQUIRE_RETURN(ppResource, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    if (_wcsicmp(pName, L"output") == 0)
    {
        hRetVal = m_pCurrentPass->m_aBuffers[!m_pCurrentPass->m_nCurrentBuffer]->QueryInterface(IID_PPV_ARGS(ppResource));
    }
    else
    {
        auto pIterator = m_aBuffers.find(pName);
        if (pIterator != m_aBuffers.end())
        {
            hRetVal = (*pIterator).second.m_spResource->QueryInterface(IID_PPV_ARGS(ppResource));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKRenderSystem::FlipCurrentBuffers(void)
{
    REQUIRE_VOID_RETURN(m_pCurrentPass);
    m_pCurrentPass->m_nCurrentBuffer = !m_pCurrentPass->m_nCurrentBuffer;
}

STDMETHODIMP_(void) CGEKRenderSystem::SetScreenTargets(IGEK3DVideoContext *pContext, IUnknown *pDepthBuffer)
{
    REQUIRE_VOID_RETURN(m_pSystem && m_pCurrentPass);

    pContext->SetRenderTargets({ m_pCurrentPass->m_aBuffers[m_pCurrentPass->m_nCurrentBuffer] }, (pDepthBuffer ? pDepthBuffer : nullptr));

    GEK3DVIDEO::VIEWPORT kViewport;
    kViewport.m_nTopLeftX = 0.0f;
    kViewport.m_nTopLeftY = 0.0f;
    kViewport.m_nXSize = float(m_pSystem->GetXSize());
    kViewport.m_nYSize = float(m_pSystem->GetYSize());
    kViewport.m_nMinDepth = 0.0f;
    kViewport.m_nMaxDepth = 1.0f;
    pContext->SetViewports({ kViewport });
}

STDMETHODIMP CGEKRenderSystem::LoadMaterial(LPCWSTR pName, IUnknown **ppMaterial)
{
    REQUIRE_RETURN(pName && ppMaterial, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppMaterial));
    }
    else
    {
        CComPtr<IGEKRenderMaterial> spMaterial;
        GetContext()->CreateInstance(CLSID_GEKRenderMaterial, IID_PPV_ARGS(&spMaterial));
        if (spMaterial)
        {
            hRetVal = spMaterial->Load(pName);
            if (SUCCEEDED(hRetVal))
            {
                m_aResources[pName] = spMaterial;
                hRetVal = spMaterial->QueryInterface(IID_PPV_ARGS(ppMaterial));
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(bool) CGEKRenderSystem::EnableMaterial(IGEK3DVideoContext *pContext, IUnknown *pMaterial)
{
    REQUIRE_RETURN(m_pCurrentFilter && pContext && pMaterial, false);

    bool bEnabled = false;
    CComQIPtr<IGEKRenderMaterial> spMaterial(pMaterial);
    if (spMaterial)
    {
        bEnabled = spMaterial->Enable(pContext, m_pCurrentFilter->m_strMaterial);
/*
            MATERIALBUFFER kMaterial;
            kMaterial.m_nColor = spMaterial->GetColor();
            kMaterial.m_bFullBright = spMaterial->IsFullBright();
            m_spMaterialBuffer->Update((void *)&kMaterial);
*/
    }

    return bEnabled;
}

STDMETHODIMP CGEKRenderSystem::LoadProgram(LPCWSTR pName, IUnknown **ppProgram)
{
    REQUIRE_RETURN(pName && ppProgram, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aResources.find(pName);
    if (pIterator != m_aResources.end())
    {
        hRetVal = (*pIterator).second->QueryInterface(IID_PPV_ARGS(ppProgram));
    }
    else
    {
        CStringA strDeferredProgram;
        hRetVal = GEKLoadFromFile(L"%root%\\data\\programs\\vertex\\plugin.hlsl", strDeferredProgram);
        if (SUCCEEDED(hRetVal))
        {
            if (strDeferredProgram.Find("_INSERT_WORLD_PROGRAM") < 0)
            {
                hRetVal = E_FAIL;
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

            LIGHTBUFFER *pLights = nullptr;
            if (SUCCEEDED(m_spLightBuffer->Map((LPVOID *)&pLights)))
            {
                memcpy(pLights, &m_aVisibleLights[nPass], (sizeof(LIGHTBUFFER)* nNumLights));
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
    spContext->GetVertexSystem()->SetSamplerStates(0, m_spPointSampler);
    spContext->GetVertexSystem()->SetSamplerStates(1, m_spLinearClampSampler);
    spContext->GetPixelSystem()->SetSamplerStates(0, m_spPointSampler);
    spContext->GetPixelSystem()->SetSamplerStates(1, m_spLinearWrapSampler);
    m_pSceneManager->ListComponentsEntities({ GET_COMPONENT_ID(transform), GET_COMPONENT_ID(viewer) }, [&](const GEKENTITYID &nViewerID) -> void
    {
        auto &kViewer = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(viewer)>(nViewerID, GET_COMPONENT_ID(viewer));
        if (SUCCEEDED(LoadPass(kViewer.pass)))
        {
            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnRenderBegin, std::placeholders::_1)));

            float4x4 nCameraMatrix;
            auto &kTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nViewerID, GET_COMPONENT_ID(transform));
            nCameraMatrix = kTransform.rotation;
            nCameraMatrix.t = kTransform.position;

            float nXSize = float(m_pSystem->GetXSize());
            float nYSize = float(m_pSystem->GetYSize());
            float nAspect = (nXSize / nYSize);

            ENGINEBUFFER kEngineBuffer;
            float nFieldOfView = _DEGTORAD(kViewer.fieldofview);
            kEngineBuffer.m_nCameraFieldOfView.x = tan(nFieldOfView * 0.5f);
            kEngineBuffer.m_nCameraFieldOfView.y = (kEngineBuffer.m_nCameraFieldOfView.x / nAspect);
            kEngineBuffer.m_nCameraMinDistance = kViewer.mindistance;
            kEngineBuffer.m_nCameraMaxDistance = kViewer.maxdistance;

            kEngineBuffer.m_nViewMatrix = nCameraMatrix.GetInverse();
            kEngineBuffer.m_nProjectionMatrix.SetPerspective(nFieldOfView, nAspect, kViewer.mindistance, kViewer.maxdistance);
            kEngineBuffer.m_nInvProjectionMatrix = kEngineBuffer.m_nProjectionMatrix.GetInverse();
            kEngineBuffer.m_nTransformMatrix = (kEngineBuffer.m_nViewMatrix * kEngineBuffer.m_nProjectionMatrix);

            frustum nViewFrustum;
            nViewFrustum.Create(nCameraMatrix, kEngineBuffer.m_nProjectionMatrix);

            LIGHTBUFFER kData;
            m_aVisibleLights.clear();
            m_pSceneManager->ListComponentsEntities({ GET_COMPONENT_ID(transform), GET_COMPONENT_ID(light) }, [&](const GEKENTITYID &nEntityID) -> void
            {
                auto &kLight = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(light)>(nEntityID, GET_COMPONENT_ID(light));
                auto &kTransform = m_pSceneManager->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
                if (nViewFrustum.IsVisible(sphere(kTransform.position, kLight.range)))
                {
                    kData.m_nPosition = (kEngineBuffer.m_nViewMatrix * float4(kTransform.position, 1.0f));

                    kData.m_nInvRange = (1.0f / (kData.m_nRange = kLight.range));

                    kData.m_nColor = kLight.color;

                    m_aVisibleLights.push_back(kData);
                }
            });

            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnCullScene, std::placeholders::_1, nViewFrustum)));

            m_spEngineBuffer->Update((void *)&kEngineBuffer);
            spContext->GetGeometrySystem()->SetConstantBuffer(0, m_spEngineBuffer);

            spContext->GetVertexSystem()->SetConstantBuffer(0, m_spEngineBuffer);
            spContext->GetVertexSystem()->SetConstantBuffer(1, m_spOrthoBuffer);

            spContext->GetComputeSystem()->SetConstantBuffer(0, m_spEngineBuffer);
            spContext->GetComputeSystem()->SetConstantBuffer(1, m_spLightCountBuffer);

            spContext->GetPixelSystem()->SetConstantBuffer(0, m_spEngineBuffer);
            spContext->GetPixelSystem()->SetConstantBuffer(1, m_spMaterialBuffer);
            spContext->GetPixelSystem()->SetConstantBuffer(2, m_spLightCountBuffer);

            m_pCurrentPass = &m_aPasses[kViewer.pass];
            m_pCurrentPass->m_nCurrentBuffer = 0;

            for (auto &kFilter : m_pCurrentPass->m_aFilters)
            {
                m_pCurrentFilter = &kFilter;
                kFilter.m_spFilter->Draw(spContext);
                m_pCurrentFilter = nullptr;
                m_spDefaultRenderStates.Release();
                m_spDefaultBlendStates.Release();
            }

            CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnRenderEnd, std::placeholders::_1)));

            GEK3DVIDEO::VIEWPORT kViewport;
            kViewport.m_nTopLeftX = (kViewer.position.x * m_pSystem->GetXSize());
            kViewport.m_nTopLeftY = (kViewer.position.y * m_pSystem->GetYSize());
            kViewport.m_nXSize = (kViewer.size.x * m_pSystem->GetXSize());
            kViewport.m_nYSize = (kViewer.size.y * m_pSystem->GetYSize());
            kViewport.m_nMinDepth = 0.0f;
            kViewport.m_nMaxDepth = 1.0f;

            m_pVideoSystem->SetDefaultTargets(spContext);
            spContext->SetViewports({ kViewport });

            spContext->SetRenderStates(m_spRenderStates);
            spContext->SetBlendStates(float4(1.0f), 0xFFFFFFFF, m_spBlendStates);
            spContext->SetDepthStates(0x0, m_spDepthStates);
            spContext->GetComputeSystem()->SetProgram(nullptr);
            spContext->GetVertexSystem()->SetProgram(m_spVertexProgram);
            spContext->GetVertexSystem()->SetConstantBuffer(1, m_spOrthoBuffer);
            spContext->GetGeometrySystem()->SetProgram(nullptr);
            spContext->GetPixelSystem()->SetProgram(m_spPixelProgram);
            spContext->GetPixelSystem()->SetResource(0, m_pCurrentPass->m_aBuffers[m_pCurrentPass->m_nCurrentBuffer]);
            SetResource(spContext->GetPixelSystem(), 1, nullptr);
            spContext->SetVertexBuffer(0, 0, m_spVertexBuffer);
            spContext->SetIndexBuffer(0, m_spIndexBuffer);
            spContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);
            spContext->DrawIndexedPrimitive(6, 0, 0);

            m_pCurrentPass = nullptr;
        }
    });

    spContext->ClearResources();
    CGEKObservable::SendEvent(TGEKEvent<IGEKRenderObserver>(std::bind(&IGEKRenderObserver::OnRenderOverlay, std::placeholders::_1)));
    m_pVideoSystem->Present(true);

    while (!m_pVideoSystem->IsEventSet(m_spFrameEvent))
    {
        Sleep(0);
    };

    m_pVideoSystem->SetEvent(m_spFrameEvent);
}
