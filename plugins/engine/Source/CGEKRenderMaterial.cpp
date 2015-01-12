#include "CGEKRenderMaterial.h"
#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"

BEGIN_INTERFACE_LIST(CGEKRenderMaterial)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderMaterial)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderMaterial)

CGEKRenderMaterial::CGEKRenderMaterial(void)
    : m_pVideoSystem(nullptr)
    , m_pRenderManager(nullptr)
{
}

CGEKRenderMaterial::~CGEKRenderMaterial(void)
{
}

// IGEKUnknown
STDMETHODIMP CGEKRenderMaterial::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    m_pVideoSystem = GetContext()->GetCachedClass<IGEK3DVideoSystem>(CLSID_GEKVideoSystem);
    m_pRenderManager = GetContext()->GetCachedClass<IGEKRenderSystem>(CLSID_GEKRenderSystem);
    if (m_pVideoSystem != nullptr && m_pRenderManager != nullptr)
    {
        hRetVal = S_OK;
    }

    return hRetVal;
}

// IGEKRenderMaterial
STDMETHODIMP CGEKRenderMaterial::Load(LPCWSTR pName)
{
    REQUIRE_RETURN(m_pRenderManager, E_FAIL);
    REQUIRE_RETURN(pName, E_INVALIDARG);

    m_aLayers.clear();
    CLibXMLDoc kDocument;
    HRESULT hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\materials\\%s.xml", pName));
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_INVALIDARG;
        CLibXMLNode kMaterialNode = kDocument.GetRoot();
        if (kMaterialNode)
        {
            CLibXMLNode kRenderNode = kMaterialNode.FirstChildElement(L"render");
            if (kRenderNode)
            {
                hRetVal = S_OK;
                CPathW kDirectory(pName);
                kDirectory.RemoveFileSpec();
                CLibXMLNode kLayerNode = kRenderNode.FirstChildElement();
                while (kLayerNode)
                {
                    LAYER &kLayerData = m_aLayers[kLayerNode.GetType()];
                    if (kLayerNode.HasAttribute(L"fullbright"))
                    {
                        kLayerData.m_bFullBright = StrToBoolean(kLayerNode.GetAttribute(L"fullbright"));
                    }

                    if (kLayerNode.HasAttribute(L"color"))
                    {
                        kLayerData.m_nColor = StrToFloat4(kLayerNode.GetAttribute(L"color"));
                    }

                    if (kLayerNode.HasChildElement(L"states"))
                    {
                        hRetVal = kLayerData.m_kRenderStates.Load(m_pVideoSystem, kLayerNode.FirstChildElement(L"states"));
                        if (FAILED(hRetVal))
                        {
                            break;
                        }
                    }

                    if (kLayerNode.HasChildElement(L"blend"))
                    {
                        hRetVal = kLayerData.m_kBlendStates.Load(m_pVideoSystem, kLayerNode.FirstChildElement(L"blend"));
                        if (FAILED(hRetVal))
                        {
                            break;
                        }
                    }

                    CLibXMLNode kDataNodes = kLayerNode.FirstChildElement(L"data");
                    if (kDataNodes)
                    {
                        CLibXMLNode kDataNode = kDataNodes.FirstChildElement();
                        while (kDataNode)
                        {
                            CStringW strSource(kDataNode.GetAttribute(L"source"));
                            strSource.Replace(L"%material%", pName);
                            strSource.Replace(L"%directory%", kDirectory.m_strPath.GetString());

                            CComPtr<IUnknown> spData;
                            m_pRenderManager->LoadResource(strSource, &spData);
                            if (spData)
                            {
                                kLayerData.m_aData[kDataNode.GetType()] = spData;
                            }

                            kDataNode = kDataNode.NextSiblingElement();
                        };
                    }

                    kLayerNode = kLayerNode.NextSiblingElement();
                };
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(bool) CGEKRenderMaterial::Enable(IGEK3DVideoContext *pContext, LPCWSTR pLayer, const std::vector<CStringW> &aData)
{
    bool bHasData = false;
    auto pLayerIterator = m_aLayers.find(pLayer);
    if (pLayerIterator != m_aLayers.end())
    {
        bHasData = true;

        LAYER &kLayer = (*pLayerIterator).second;
        kLayer.m_kRenderStates.Enable(pContext);
        kLayer.m_kBlendStates.Enable(pContext);

        for (UINT32 nStage = 0; nStage < aData.size(); nStage++)
        {
            auto pDataIterator = kLayer.m_aData.find(aData[nStage]);
            if (pDataIterator != kLayer.m_aData.end())
            {
                m_pRenderManager->SetResource(pContext->GetPixelSystem(), nStage, (*pDataIterator).second);
            }
            else
            {
                m_pRenderManager->SetResource(pContext->GetPixelSystem(), nStage, nullptr);
            }
        }
    }

    return bHasData;
}
