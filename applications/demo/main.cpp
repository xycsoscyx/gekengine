#include <Windows.h>
#include <Commctrl.h>
#include <initguid.h>
#include <cguid.h>
#include <random>

#include "resource.h"

#include "GEKMath.h"
#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKEngine.h"

#include "GEKEngineCLSIDs.h"

static CGEKConfig kConfig;
INT_PTR CALLBACK DialogProc(HWND hDialog, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
    switch (nMessage)
    {
    case WM_CLOSE:
        EndDialog(hDialog, IDCANCEL);
        return TRUE;

    case WM_INITDIALOG:
        {
            kConfig.Load(L"%root%\\config.cfg");

            UINT32 nSelectID = 0;
            SendDlgItemMessage(hDialog, IDC_MODES, CB_RESETCONTENT, 0, 0);
            std::vector<GEKMODE> akModes = GEKGetDisplayModes()[32];
            for(UINT32 nMode = 0; nMode < akModes.size(); nMode++)
            {
                GEKMODE &kMode = akModes[nMode];

                CStringW strAspect(L"");
                switch (kMode.GetAspect())
                {
                case _ASPECT_4x3:
                    strAspect = L", (4x3)";
                    break;

                case _ASPECT_16x9:
                    strAspect = L", (16x9)";
                    break;

                case _ASPECT_16x10:
                    strAspect = L", (16x10)";
                    break;
                };

                CStringW strMode;
                strMode.Format(L"%dx%d%s", kMode.xsize, kMode.ysize, strAspect.GetString());
                int nID = SendDlgItemMessage(hDialog, IDC_MODES, CB_ADDSTRING, 0, (WPARAM)strMode.GetBuffer(0));
                if (kMode.xsize == StrToUINT32(kConfig.GetValue(L"video", L"xsize", L"640")) &&
                    kMode.ysize == StrToUINT32(kConfig.GetValue(L"video", L"ysize", L"480")))
                {
                    nSelectID = nID;
                }
            }

            SendDlgItemMessage(hDialog, IDC_FULLSCREEN, BM_SETCHECK, (StrToUINT32(kConfig.GetValue(L"video", L"windowed", L"0")) ? BST_UNCHECKED : BST_CHECKED), 0);

            SendDlgItemMessage(hDialog, IDC_MODES, CB_SETMINVISIBLE, 5, 0);
            SendDlgItemMessage(hDialog, IDC_MODES, CB_SETEXTENDEDUI, TRUE, 0);
            SendDlgItemMessage(hDialog, IDC_MODES, CB_SETCURSEL, nSelectID, 0);
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                std::vector<GEKMODE> akModes = GEKGetDisplayModes()[32];
                UINT32 nMode = SendDlgItemMessage(hDialog, IDC_MODES, CB_GETCURSEL, 0, 0);
                GEKMODE &kMode = akModes[nMode];

                kConfig.SetValue(L"video", L"xsize", L"%d", kMode.xsize);
                kConfig.SetValue(L"video", L"ysize", L"%d", kMode.ysize);
                kConfig.SetValue(L"video", L"windowed", (SendDlgItemMessage(hDialog, IDC_FULLSCREEN, BM_GETCHECK, 0, 0) == BST_CHECKED ? L"0" : L"1"));

                kConfig.Save(L"%root%\\config.cfg");

                EndDialog(hDialog, IDOK);
                return TRUE;
            }

        case IDCANCEL:
            EndDialog(hDialog, IDCANCEL);
            return TRUE;
        };

        return TRUE;
    };

    return FALSE;
}

float lerp(float nX, float nY, float nS)
{
    return ((nX * (1.0f - nS)) + (nY * nS));
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR strCommandLine, int nCmdShow)
{
    UINT32 nSize = sizeof(aabb);

    const UINT32 nNumSamples = 16;

    std::random_device kRandomDevice;
    std::mt19937 kMersine(kRandomDevice());

    std::normal_distribution<float> nRandom(-1.0f, 1.0f);

    CStringA strKernel;
    for(UINT32 nIndex = 0; nIndex < nNumSamples; nIndex++)
    {
        float3 nSample = StrToFloat3(L"rand(-1:1), rand(-1:1), rand(0:1)");
        nSample.Normalize();

        float nScale = (float(nIndex) / float(nNumSamples));
        nScale = lerp(0.1f, 1.0f, (nScale * nScale));
        nSample *= nScale;

        CStringA strSample;
        strSample.Format("                float3(%f, %f, %f),\r\n", nSample.x, nSample.y, nSample.z);
        strKernel += strSample;
    }

    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, DialogProc) == IDOK)
    {
        CComPtr<IGEKContext> spContext;
        GEKCreateContext(&spContext);
        if (spContext != nullptr)
        {
#ifdef _DEBUG
            SetCurrentDirectory(GEKParseFileName(L"%root%\\Debug"));
            spContext->AddSearchPath(GEKParseFileName(L"%root%\\Debug"));
#else
            SetCurrentDirectory(GEKParseFileName(L"%root%\\Release"));
            spContext->AddSearchPath(GEKParseFileName(L"%root%\\Release"));
#endif

            if (SUCCEEDED(spContext->Initialize()))
            {
                CComPtr<IGEKGameApplication> spGame;
                spContext->CreateInstance(CLSID_GEKEngine, IID_PPV_ARGS(&spGame));
                if (spGame != nullptr)
                {
                    spGame->Run();
                }
            }
        }

        return 0;
    }

    return -1;
}