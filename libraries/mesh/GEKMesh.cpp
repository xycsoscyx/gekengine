#include "GEKMesh.h"
#include <atlbase.h>

#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

HRESULT GEKOptimizeMesh(const VERTEX *pInputVertices, UINT32 nNumVertices, const UINT16 *pInputIndices, UINT32 nNumIndices,
                        std::vector<VERTEX> &aOutputVertices, std::vector<UINT16> &aOutputIndices,
                        float nFaceEpsilon, float nPartialEdgeThreshold, float nSingularPointThreshold, float nNormalEdgeThreshold)
{
    REQUIRE_RETURN(pInputVertices, E_INVALIDARG);
    REQUIRE_RETURN(nNumVertices > 0, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;

    std::vector<UINT16> aInputIndices;
    if (pInputIndices && nNumIndices > 0)
    {
        aInputIndices.assign(pInputIndices, &pInputIndices[nNumIndices]);
    }
    else
    {
        aInputIndices.resize(nNumVertices);
        for (UINT32 nIndex = 0; nIndex < nNumVertices; nIndex++)
        {
            aInputIndices[nIndex] = nIndex;
        }
    }

    WNDCLASSEX kWindowClass;
    kWindowClass.cbSize = sizeof(WNDCLASSEX);
    kWindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    kWindowClass.lpfnWndProc = (WNDPROC)DefWindowProcW;
    kWindowClass.cbWndExtra = 0;
    kWindowClass.cbClsExtra = 0;
    kWindowClass.hInstance = GetModuleHandle(nullptr);
    kWindowClass.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    kWindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    kWindowClass.hbrBackground = nullptr;
    kWindowClass.lpszMenuName = nullptr;
    kWindowClass.lpszClassName = L"GEK.Optimizer";
    kWindowClass.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);
    if (RegisterClassEx(&kWindowClass))
    {
        HWND hWindow = CreateWindowEx(0, L"GEK.Optimizer", L"GEK Optimizer", WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        if (hWindow)
        {
            CComPtr<IDirect3D9> spDirect3D = Direct3DCreate9(D3D_SDK_VERSION);
            if (spDirect3D)
            {
                D3DPRESENT_PARAMETERS kDirect3DParams;
                ZeroMemory(&kDirect3DParams, sizeof(kDirect3DParams));
                kDirect3DParams.Windowed = true;
                kDirect3DParams.hDeviceWindow = hWindow;
                kDirect3DParams.BackBufferCount = 1;
                kDirect3DParams.BackBufferWidth = 1;
                kDirect3DParams.BackBufferHeight = 1;
                kDirect3DParams.BackBufferFormat = D3DFMT_X8R8G8B8;
                kDirect3DParams.SwapEffect = D3DSWAPEFFECT_DISCARD;

                CComPtr<IDirect3DDevice9> spDevice;
                spDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &kDirect3DParams, &spDevice);
                if (spDevice)
                {
                    D3DVERTEXELEMENT9 aElements[] =
                    {
                        { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
                        { 0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
                        { 0, 20, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0 },
                        { 0, 32, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0 },
                        { 0, 44, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
                        D3DDECL_END(),
                    };

                    CComPtr<ID3DXMesh> spMesh;
                    hRetVal = D3DXCreateMesh((aInputIndices.size() / 3), nNumVertices, D3DXMESH_MANAGED, aElements, spDevice, &spMesh);
                    if (spMesh)
                    {
                        VERTEX *pVertices = nullptr;
                        spMesh->LockVertexBuffer(D3DLOCK_DISCARD, (void **)&pVertices);
                        memcpy(pVertices, pInputVertices, (sizeof(VERTEX) * nNumVertices));
                        spMesh->UnlockVertexBuffer();

                        UINT16 *pIndices = nullptr;
                        spMesh->LockIndexBuffer(D3DLOCK_DISCARD, (void **)&pIndices);
                        memcpy(pIndices, &aInputIndices[0], (sizeof(UINT16) * aInputIndices.size()));
                        spMesh->UnlockIndexBuffer();

                        std::vector<DWORD> aAdjacency(aInputIndices.size());
                        hRetVal = spMesh->GenerateAdjacency(nFaceEpsilon, &aAdjacency[0]);
                        if (SUCCEEDED(hRetVal))
                        {
                            std::vector<DWORD> aOptimizedAdjacency(aInputIndices.size());
                            hRetVal = spMesh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_VERTEXCACHE, &aAdjacency[0], &aOptimizedAdjacency[0], nullptr, nullptr);
                            if (SUCCEEDED(hRetVal))
                            {
                                CComPtr<ID3DXMesh> spFinalMesh;
                                hRetVal = D3DXComputeTangentFrameEx(spMesh, D3DDECLUSAGE_TEXCOORD, 0, D3DDECLUSAGE_TANGENT, 0, D3DDECLUSAGE_BINORMAL, 0,
                                    D3DDECLUSAGE_NORMAL, 0, D3DXTANGENT_CALCULATE_NORMALS,
                                    &aOptimizedAdjacency[0], nPartialEdgeThreshold, nSingularPointThreshold, nNormalEdgeThreshold, &spFinalMesh, nullptr);
                                if (spFinalMesh)
                                {
                                    spFinalMesh->LockVertexBuffer(D3DLOCK_READONLY, (void **)&pVertices);
                                    aOutputVertices.assign(pVertices, &pVertices[spFinalMesh->GetNumVertices()]);
                                    spFinalMesh->UnlockVertexBuffer();

                                    spFinalMesh->LockIndexBuffer(D3DLOCK_READONLY, (void **)&pIndices);
                                    aOutputIndices.assign(pIndices, &pIndices[spFinalMesh->GetNumFaces() * 3]);
                                    spFinalMesh->UnlockIndexBuffer();
                                }
                            }
                        }
                    }
                }
            }

            DestroyWindow(hWindow);
            UnregisterClass(L"GEK.Optimizer", GetModuleHandle(nullptr));
        }
    }

    return hRetVal;
}