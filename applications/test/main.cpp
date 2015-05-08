#include <Windows.h>
#include <Commctrl.h>
#include <initguid.h>
#include <cguid.h>

#include "resource.h"

#include "GEKMath.h"
#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"

#include "GEKSystemCLSIDs.h"

GEKHANDLE gs_nSampleStatesID = GEKINVALIDHANDLE;
GEKHANDLE gs_nRenderStatesID = GEKINVALIDHANDLE;
GEKHANDLE gs_nBlendStatesID = GEKINVALIDHANDLE;
GEKHANDLE gs_nDepthStatesID = GEKINVALIDHANDLE;
GEKHANDLE gs_nVertexProgramID = GEKINVALIDHANDLE;
GEKHANDLE gs_nPixelProgramID = GEKINVALIDHANDLE;
GEKHANDLE gs_nConstantBufferID = GEKINVALIDHANDLE;
GEKHANDLE gs_nVertexBufferID = GEKINVALIDHANDLE;
GEKHANDLE gs_nIndexBufferID = GEKINVALIDHANDLE;
GEKHANDLE gs_nTextureID = GEKINVALIDHANDLE;
CComAutoCriticalSection m_kSection;

INT_PTR CALLBACK DialogProc(HWND hDialog, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
    switch (nMessage)
    {
    case WM_CLOSE:
        if (true)
        {
            CComPtr<IGEK3DVideoSystem> spVideoSystem;
            spVideoSystem.Attach((IGEK3DVideoSystem *)GetWindowLongPtr(hDialog, GWLP_USERDATA));
            EndDialog(hDialog, IDCANCEL);
        }

        return TRUE;

    case WM_INITDIALOG:
        if (true)
        {
            IGEKContext *pContext = (IGEKContext *)lParam;

            CComPtr<IGEK3DVideoSystem> spVideoSystem;
            pContext->CreateInstance(CLSID_GEKVideoSystem, IID_PPV_ARGS(&spVideoSystem));
            if (spVideoSystem)
            {
                HWND hWindow = GetDlgItem(hDialog, IDC_VIDEO_WINDOW);

                RECT kRect;
                GetClientRect(hWindow, &kRect);
                spVideoSystem->Initialize(hWindow, false, kRect.right, kRect.bottom, GEK3DVIDEO::DATA::D24_S8);

                static const float2 aVertices[] =
                {
                    float2(0.0f, 0.0f),
                    float2(1.0f, 0.0f),
                    float2(1.0f, 1.0f),
                    float2(0.0f, 1.0f),
                };

                static const UINT16 aIndices[6] =
                {
                    0, 1, 2,
                    0, 2, 3,
                };

                GEK3DVIDEO::SAMPLERSTATES kStates;
                gs_nSampleStatesID = spVideoSystem->CreateSamplerStates(kStates);

                GEK3DVIDEO::RENDERSTATES kRenderStates;
                gs_nRenderStatesID = spVideoSystem->CreateRenderStates(kRenderStates);

                GEK3DVIDEO::UNIFIEDBLENDSTATES kBlendStates;
                gs_nBlendStatesID = spVideoSystem->CreateBlendStates(kBlendStates);

                GEK3DVIDEO::DEPTHSTATES kDepthStates;
                gs_nDepthStatesID = spVideoSystem->CreateDepthStates(kDepthStates);

                gs_nVertexBufferID = spVideoSystem->CreateBuffer(sizeof(float2), 4, GEK3DVIDEO::BUFFER::VERTEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, aVertices);

                gs_nIndexBufferID = spVideoSystem->CreateBuffer(sizeof(UINT16), 6, GEK3DVIDEO::BUFFER::INDEX_BUFFER | GEK3DVIDEO::BUFFER::STATIC, aIndices);

                gs_nTextureID = spVideoSystem->LoadTexture(L"%root%\\data\\textures\\flames.NormalMap.dds", 0);

                std::vector<GEK3DVIDEO::INPUTELEMENT> aLayout;
                aLayout.push_back(GEK3DVIDEO::INPUTELEMENT(GEK3DVIDEO::DATA::RG_FLOAT, "POSITION", 0));
                gs_nVertexProgramID = spVideoSystem->LoadVertexProgram(L"%root%\\data\\programs\\core\\gekoverlay.hlsl", "MainVertexProgram", aLayout);

                gs_nPixelProgramID = spVideoSystem->LoadPixelProgram(L"%root%\\data\\programs\\core\\gekoverlay.hlsl", "MainPixelProgram");

                gs_nConstantBufferID = spVideoSystem->CreateBuffer(sizeof(float4x4), 1, GEK3DVIDEO::BUFFER::CONSTANT_BUFFER);

                float4x4 nOverlayMatrix;
                nOverlayMatrix.SetOrthographic(0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f);
                spVideoSystem->UpdateBuffer(gs_nConstantBufferID, LPCVOID(&nOverlayMatrix));

                SetWindowLongPtr(hDialog, GWLP_USERDATA, LONG(spVideoSystem.Detach()));
            }
        }

        return TRUE;

    case WM_PAINT:
        if (true)
        {
            CComCritSecLock<CComAutoCriticalSection> kLock(m_kSection);
            IGEK3DVideoSystem *pVideoSystem = (IGEK3DVideoSystem *)GetWindowLongPtr(hDialog, GWLP_USERDATA);
            pVideoSystem->ClearDefaultRenderTarget(float4(1.0f, 0.0f, 0.0f, 1.0f));

            CComQIPtr<IGEK3DVideoContext> spVideoContext(pVideoSystem);
            pVideoSystem->SetDefaultTargets(spVideoContext);
            spVideoContext->SetRenderStates(gs_nRenderStatesID);
            spVideoContext->SetBlendStates(gs_nBlendStatesID, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xFFFFFFFF);
            spVideoContext->SetDepthStates(gs_nDepthStatesID, 0);
            spVideoContext->GetVertexSystem()->SetProgram(gs_nVertexProgramID);
            spVideoContext->GetVertexSystem()->SetConstantBuffer(gs_nConstantBufferID, 1);
            spVideoContext->GetPixelSystem()->SetProgram(gs_nPixelProgramID);
            spVideoContext->GetPixelSystem()->SetSamplerStates(gs_nSampleStatesID, 0);
            spVideoContext->GetPixelSystem()->SetResource(gs_nTextureID, 0);
            spVideoContext->SetVertexBuffer(gs_nVertexBufferID, 0, 0);
            spVideoContext->SetIndexBuffer(gs_nIndexBufferID, 0);
            spVideoContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);
            spVideoContext->DrawIndexedPrimitive(6, 0, 0);

            pVideoSystem->Present(false);
            return TRUE;
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_FILE_LOAD:
            if (true)
            {
                CStringW strFileName;
                OPENFILENAMEW kOpenFileName;
                ZeroMemory(&kOpenFileName, sizeof(OPENFILENAMEW));
                kOpenFileName.lStructSize = sizeof(OPENFILENAMEW);
                kOpenFileName.hwndOwner = hDialog;
                kOpenFileName.lpstrFile = strFileName.GetBuffer(MAX_PATH + 1);
                kOpenFileName.nMaxFile = MAX_PATH;
                kOpenFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
                if (GetOpenFileNameW((LPOPENFILENAMEW)&kOpenFileName))
                {
                    strFileName.ReleaseBuffer();
                    IGEK3DVideoSystem *pVideoSystem = (IGEK3DVideoSystem *)GetWindowLongPtr(hDialog, GWLP_USERDATA);
                    pVideoSystem->FreeResource(gs_nTextureID);
                    gs_nTextureID = pVideoSystem->LoadTexture(strFileName, 0);
                }
            }

            return TRUE;

        case ID_FILE_QUIT:
            if (true)
            {
                CComPtr<IGEK3DVideoSystem> spVideoSystem;
                spVideoSystem.Attach((IGEK3DVideoSystem *)GetWindowLongPtr(hDialog, GWLP_USERDATA));
                EndDialog(hDialog, IDCANCEL);
            }

            return TRUE;
        };

        return TRUE;
    };

    return FALSE;
}

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR strCommandLine, _In_ int nCmdShow)
{
    CComPtr<IGEKContext> spContext;
    GEKCreateContext(&spContext);
    if (spContext)
    {
#ifdef _DEBUG
        SetCurrentDirectory(GEKParseFileName(L"%root%\\Debug"));
        spContext->AddSearchPath(GEKParseFileName(L"%root%\\Debug\\Plugins"));
#else
        SetCurrentDirectory(GEKParseFileName(L"%root%\\Release"));
        spContext->AddSearchPath(GEKParseFileName(L"%root%\\Release\\Plugins"));
#endif

        if (SUCCEEDED(spContext->Initialize()))
        {
            DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_TEST_DIALOG), nullptr, DialogProc, LPARAM((IGEKContext *)spContext));
        }
    }

    return 0;
}