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

            CComQIPtr<IGEK3DVideoContext> spContext(pVideoSystem);
            pVideoSystem->SetDefaultTargets(spContext);
            spContext->GetVertexSystem()->SetProgram(gs_nVertexProgramID);
            spContext->GetVertexSystem()->SetConstantBuffer(gs_nConstantBufferID, 1);
            spContext->GetPixelSystem()->SetProgram(gs_nPixelProgramID);
            spContext->GetPixelSystem()->SetResource(gs_nTextureID, 0);
            spContext->SetVertexBuffer(gs_nVertexBufferID, 0, 0);
            spContext->SetIndexBuffer(gs_nIndexBufferID, 0);
            spContext->SetPrimitiveType(GEK3DVIDEO::PRIMITIVE::TRIANGLELIST);
            spContext->DrawIndexedPrimitive(6, 0, 0);

            pVideoSystem->Present(false);
            return TRUE;
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
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
            if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_TEST_DIALOG), nullptr, DialogProc, LPARAM((IGEKContext *)spContext)) == IDOK)
            {
            }
        }
    }

    return 0;
}