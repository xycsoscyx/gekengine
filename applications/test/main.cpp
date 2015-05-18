#include <initguid.h>
#include <cguid.h>

#include "GEK\Math\Matrix4x4.h"
#include "GEK\Utility\Common.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\Interface.h"
#include "GEK\Context\BaseUnknown.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "resource.h"

Gek::Handle gs_nSampleStatesID = Gek::InvalidHandle;
Gek::Handle gs_nRenderStatesID = Gek::InvalidHandle;
Gek::Handle gs_nBlendStatesID = Gek::InvalidHandle;
Gek::Handle gs_nDepthStatesID = Gek::InvalidHandle;
Gek::Handle gs_nVertexProgramID = Gek::InvalidHandle;
Gek::Handle gs_nPixelProgramID = Gek::InvalidHandle;
Gek::Handle gs_nConstantBufferID = Gek::InvalidHandle;
Gek::Handle gs_nVertexBufferID = Gek::InvalidHandle;
Gek::Handle gs_nIndexBufferID = Gek::InvalidHandle;
Gek::Handle gs_nTextureID = Gek::InvalidHandle;
CComAutoCriticalSection criticalSection;

INT_PTR CALLBACK dialogProcedure(HWND dialogWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        if (true)
        {
            CComPtr<Gek::Video3D::Interface> videoSystem;
            videoSystem.Attach((Gek::Video3D::Interface *)GetWindowLongPtr(dialogWindow, GWLP_USERDATA));
            EndDialog(dialogWindow, IDCANCEL);
        }

        return TRUE;

    case WM_INITDIALOG:
        if (true)
        {
            CComQIPtr<Gek::Context::Interface> context((IUnknown *)lParam);

            CComPtr<Gek::Video3D::Interface> videoSystem;
            context->createInstance(__uuidof(Gek::Video::Class), IID_PPV_ARGS(&videoSystem));
            if (videoSystem)
            {
                HWND renderWindow = GetDlgItem(dialogWindow, IDC_VIDEO_WINDOW);

                RECT renderClientRect;
                GetClientRect(renderWindow, &renderClientRect);
                videoSystem->initialize(renderWindow, false, renderClientRect.right, renderClientRect.bottom, Gek::Video3D::Format::D24_S8);

                static const Gek::Math::Float2 aVertices[] =
                {
                    Gek::Math::Float2(0.0f, 0.0f),
                    Gek::Math::Float2(1.0f, 0.0f),
                    Gek::Math::Float2(1.0f, 1.0f),
                    Gek::Math::Float2(0.0f, 1.0f),
                };

                static const UINT16 aIndices[6] =
                {
                    0, 1, 2,
                    0, 2, 3,
                };

                Gek::Video3D::SamplerStates samplerStates;
                gs_nSampleStatesID = videoSystem->createSamplerStates(samplerStates);

                Gek::Video3D::RenderStates renderStates;
                gs_nRenderStatesID = videoSystem->createRenderStates(renderStates);

                Gek::Video3D::UnifiedBlendStates blendStates;
                gs_nBlendStatesID = videoSystem->createBlendStates(blendStates);

                Gek::Video3D::DepthStates depthStates;
                gs_nDepthStatesID = videoSystem->createDepthStates(depthStates);

                gs_nVertexBufferID = videoSystem->createBuffer(sizeof(Gek::Math::Float2), 4, Gek::Video3D::BufferFlags::VERTEX_BUFFER | Gek::Video3D::BufferFlags::STATIC, aVertices);

                gs_nIndexBufferID = videoSystem->createBuffer(sizeof(UINT16), 6, Gek::Video3D::BufferFlags::INDEX_BUFFER | Gek::Video3D::BufferFlags::STATIC, aIndices);

                gs_nTextureID = videoSystem->loadTexture(L"%root%\\data\\textures\\flames.NormalMap.dds", 0);

                std::vector<Gek::Video3D::InputElement> overlayVertexLayout;
                overlayVertexLayout.push_back(Gek::Video3D::InputElement(Gek::Video3D::Format::RG_FLOAT, "POSITION", 0));
                gs_nVertexProgramID = videoSystem->loadVertexProgram(L"%root%\\data\\programs\\core\\gekoverlay.hlsl", "MainVertexProgram", overlayVertexLayout);

                gs_nPixelProgramID = videoSystem->loadPixelProgram(L"%root%\\data\\programs\\core\\gekoverlay.hlsl", "MainPixelProgram");

                gs_nConstantBufferID = videoSystem->createBuffer(sizeof(Gek::Math::Float4x4), 1, Gek::Video3D::BufferFlags::CONSTANT_BUFFER);

                Gek::Math::Float4x4 orthoMatrix;
                orthoMatrix.setOrthographic(0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 1.0f);
                videoSystem->updateBuffer(gs_nConstantBufferID, LPCVOID(&orthoMatrix));

                SetWindowLongPtr(dialogWindow, GWLP_USERDATA, LONG(videoSystem.Detach()));
            }
        }

        return TRUE;

    case WM_SIZE:
        if (true)
        {
            CComCritSecLock<CComAutoCriticalSection> scopedLock(criticalSection);
            Gek::Video3D::Interface *videoSystem = (Gek::Video3D::Interface *)GetWindowLongPtr(dialogWindow, GWLP_USERDATA);

            RECT dialogClientRect;
            GetClientRect(dialogWindow, &dialogClientRect);

            RECT renderClientRect(dialogClientRect);
            renderClientRect.left += 10;
            renderClientRect.right -= 10;
            renderClientRect.top += 10;
            renderClientRect.bottom -= 10;
            UINT32 clientWidth = (renderClientRect.right - renderClientRect.left);
            UINT32 clientHeight = (renderClientRect.bottom - renderClientRect.top);

            HWND renderWindow = GetDlgItem(dialogWindow, IDC_VIDEO_WINDOW);
            SetWindowPos(renderWindow, nullptr, renderClientRect.left, renderClientRect.top, clientWidth, clientHeight, SWP_NOZORDER);
            videoSystem->resize(false, clientWidth, clientHeight, Gek::Video3D::Format::D24_S8);
        }
        break;

    case WM_PAINT:
        if (true)
        {
            CComCritSecLock<CComAutoCriticalSection> scopedLock(criticalSection);
            Gek::Video3D::Interface *videoSystem = (Gek::Video3D::Interface *)GetWindowLongPtr(dialogWindow, GWLP_USERDATA);
            videoSystem->clearDefaultRenderTarget(Gek::Math::Float4(1.0f, 0.0f, 0.0f, 1.0f));

            videoSystem->setDefaultTargets();
            videoSystem->getDefaultContext()->setRenderStates(gs_nRenderStatesID);
            videoSystem->getDefaultContext()->setBlendStates(gs_nBlendStatesID, Gek::Math::Float4(1.0f, 1.0f, 1.0f, 1.0f), 0xFFFFFFFF);
            videoSystem->getDefaultContext()->setDepthStates(gs_nDepthStatesID, 0);
            videoSystem->getDefaultContext()->getVertexSystem()->setProgram(gs_nVertexProgramID);
            videoSystem->getDefaultContext()->getVertexSystem()->setConstantBuffer(gs_nConstantBufferID, 1);
            videoSystem->getDefaultContext()->getPixelSystem()->setProgram(gs_nPixelProgramID);
            videoSystem->getDefaultContext()->getPixelSystem()->setSamplerStates(gs_nSampleStatesID, 0);
            videoSystem->getDefaultContext()->getPixelSystem()->setResource(gs_nTextureID, 0);
            videoSystem->getDefaultContext()->setVertexBuffer(gs_nVertexBufferID, 0, 0);
            videoSystem->getDefaultContext()->setIndexBuffer(gs_nIndexBufferID, 0);
            videoSystem->getDefaultContext()->setPrimitiveType(Gek::Video3D::PrimitiveType::TRIANGLELIST);
            videoSystem->getDefaultContext()->drawIndexedPrimitive(6, 0, 0);

            videoSystem->present(false);
            return TRUE;
        }

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_FILE_LOAD:
            if (true)
            {
                CStringW fileName;
                OPENFILENAMEW openFileName;
                ZeroMemory(&openFileName, sizeof(OPENFILENAMEW));
                openFileName.lStructSize = sizeof(OPENFILENAMEW);
                openFileName.hwndOwner = dialogWindow;
                openFileName.lpstrFile = fileName.GetBuffer(MAX_PATH + 1);
                openFileName.nMaxFile = MAX_PATH;
                openFileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
                if (GetOpenFileNameW((LPOPENFILENAMEW)&openFileName))
                {
                    fileName.ReleaseBuffer();
                    Gek::Video3D::Interface *videoSystem = (Gek::Video3D::Interface *)GetWindowLongPtr(dialogWindow, GWLP_USERDATA);
                    videoSystem->freeResource(gs_nTextureID);
                    gs_nTextureID = videoSystem->loadTexture(fileName, 0);
                }
            }

            return TRUE;

        case ID_FILE_QUIT:
            if (true)
            {
                CComPtr<Gek::Video3D::Interface> videoSystem;
                videoSystem.Attach((Gek::Video3D::Interface *)GetWindowLongPtr(dialogWindow, GWLP_USERDATA));
                EndDialog(dialogWindow, IDCANCEL);
            }

            return TRUE;
        };

        return TRUE;
    };

    return FALSE;
}

namespace Gek
{
    class Initializer : public BaseUnknown
    {
    private:
        CComPtr<Gek::Context::Interface> context;
        CComPtr<Gek::Engine::Population::Interface> population;

    public:
        Initializer(void)
        {
        }

        virtual ~Initializer(void)
        {
        }

        STDMETHODIMP initialize(void)
        {
            HRESULT resultValue = Gek::Context::create(&context);
            if (context)
            {
#ifdef _DEBUG
                SetCurrentDirectory(Gek::FileSystem::expandPath(L"%root%\\Debug"));
                context->addSearchPath(L"%root%\\Debug\\Plugins");
#else
                SetCurrentDirectory(GEKParseFileName(L"%root%\\Release"));
                context->AddSearchPath(GEKParseFileName(L"%root%\\Release\\Plugins"));
#endif

                context->initialize();
                resultValue = context->createInstance(__uuidof(Gek::Engine::Population::Class), IID_PPV_ARGS(&population));
            }

            if (SUCCEEDED(resultValue))
            {
                resultValue = population->initialize();
            }

            return resultValue;
        }

        // IUnknown
        BEGIN_INTERFACE_LIST(Initializer)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Gek::Context::Interface, context)
            INTERFACE_LIST_ENTRY_MEMBER_COM(Gek::Engine::Population::Interface, population)
        END_INTERFACE_LIST_UNKNOWN
    };
}; // namespace Gek

int CALLBACK wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previousInstance, _In_ LPWSTR commandLine, _In_ int commandShow)
{
    CComPtr<Gek::Initializer> initializer(new Gek::Initializer());
    if (initializer && SUCCEEDED(initializer->initialize()))
    {
        DialogBoxParam(instance, MAKEINTRESOURCE(IDD_TEST_DIALOG), nullptr, dialogProcedure, LPARAM(initializer->getUnknown()));
    }

    return 0;
}