#include "CGEKRenderMaterial.h"
#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"

BEGIN_INTERFACE_LIST(CGEKRenderMaterial)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderMaterial)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKRenderMaterial)

CGEKRenderMaterial::CGEKRenderMaterial(void)
    : m_pEngine(nullptr)
    , m_pRenderSystem(nullptr)
{
}

CGEKRenderMaterial::~CGEKRenderMaterial(void)
{
}

// IGEKRenderMaterial
STDMETHODIMP CGEKRenderMaterial::Load(IGEKEngineCore *pEngine, IGEKRenderSystem *pRenderSystem, LPCWSTR pName)
{
    REQUIRE_RETURN(pEngine && pRenderSystem && pName, E_INVALIDARG);

    m_pEngine = pEngine;
    m_pRenderSystem = pRenderSystem;

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
                        hRetVal = kLayerData.m_kRenderStates.Load(m_pEngine->GetVideoSystem(), kLayerNode.FirstChildElement(L"states"));
                        if (FAILED(hRetVal))
                        {
                            break;
                        }
                    }

                    if (kLayerNode.HasChildElement(L"blend"))
                    {
                        hRetVal = kLayerData.m_kBlendStates.Load(m_pEngine->GetVideoSystem(), kLayerNode.FirstChildElement(L"blend"));
                        if (FAILED(hRetVal))
                        {
                            break;
                        }
                    }

                    CLibXMLNode kDataNode = kLayerNode.FirstChildElement(L"data");
                    if (kDataNode)
                    {
                        CLibXMLNode kStageNode = kDataNode.FirstChildElement(L"stage");
                        while (kStageNode)
                        {
                            if (!kStageNode.HasAttribute(L"source"))
                            {
                                hRetVal = E_FAIL;
                                break;
                            }

                            CStringW strSource(kStageNode.GetAttribute(L"source"));
                            strSource.Replace(L"%material%", pName);
                            strSource.Replace(L"%directory%", kDirectory.m_strPath.GetString());

                            CComPtr<IUnknown> spData;
                            m_pRenderSystem->LoadResource(strSource, &spData);
                            kLayerData.m_aData.push_back(spData);

                            kStageNode = kStageNode.NextSiblingElement();
                        };
                    }

                    kLayerNode = kLayerNode.NextSiblingElement();
                };
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(bool) CGEKRenderMaterial::Enable(IGEK3DVideoContext *pContext, LPCWSTR pLayer)
{
    bool bHasData = false;
    auto pLayerIterator = m_aLayers.find(pLayer);
    if (pLayerIterator != m_aLayers.end())
    {
        bHasData = true;

        LAYER &kLayer = (*pLayerIterator).second;
        if (!kLayer.m_kRenderStates.Enable(pContext))
        {
            m_pRenderSystem->EnableDefaultRenderStates(pContext);
        }

        if (!kLayer.m_kBlendStates.Enable(pContext))
        {
            m_pRenderSystem->EnableDefaultBlendStates(pContext);
        }

        for (UINT32 nStage = 0; nStage < kLayer.m_aData.size(); nStage++)
        {
            m_pRenderSystem->SetResource(pContext->GetPixelSystem(), nStage, kLayer.m_aData[nStage]);
        }
    }

    return bHasData;
}
