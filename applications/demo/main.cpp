#include <initguid.h>
#include <cguid.h>

#include "GEK\Utility\Common.h"
#include "GEK\Utility\Display.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Interface.h"
#include "GEK\Engine\CoreInterface.h"
#include <CommCtrl.h>
#include "resource.h"

INT_PTR CALLBACK DialogProc(HWND hDialog, UINT nMessage, WPARAM wParam, LPARAM lParam)
{
    switch (nMessage)
    {
    case WM_CLOSE:
        EndDialog(hDialog, IDCANCEL);
        return TRUE;

    case WM_INITDIALOG:
        {
            UINT32 width = 800;
            UINT32 height = 600;
            bool windowed = true;

            Gek::Xml::Document xmlDocument;
            if (SUCCEEDED(xmlDocument.load(L"%root%\\config.xml")))
            {
                Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                if (xmlConfigNode && xmlConfigNode.getType().CompareNoCase(L"config") == 0 && xmlConfigNode.hasChildElement(L"display"))
                {
                    Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                    if (xmlDisplayNode)
                    {
                        if (xmlDisplayNode.hasAttribute(L"xsize"))
                        {
                            width = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"xsize"));
                        }
                        
                        if (xmlDisplayNode.hasAttribute(L"ysize"))
                        {
                            height = Gek::String::getUINT32(xmlDisplayNode.getAttribute(L"ysize"));
                        }
                        
                        if (xmlDisplayNode.hasAttribute(L"windowed"))
                        {
                            windowed = Gek::String::getBoolean(xmlDisplayNode.getAttribute(L"windowed"));
                        }
                    }
                }
            }

            UINT32 selectIndex = 0;
            SendDlgItemMessage(hDialog, IDC_MODES, CB_RESETCONTENT, 0, 0);
            std::vector<Gek::Display::Mode> modeList = Gek::Display::getModes()[32];
            for(auto &mode : modeList)
            {
                CStringW strAspect(L"");
                switch (mode.aspectRatio)
                {
                case Gek::Display::AspectRatio::_4x3:
                    strAspect = L", (4x3)";
                    break;

                case Gek::Display::AspectRatio::_16x9:
                    strAspect = L", (16x9)";
                    break;

                case Gek::Display::AspectRatio::_16x10:
                    strAspect = L", (16x10)";
                    break;
                };

                CStringW modeString;
                modeString.Format(L"%dx%d%s", mode.width, mode.height, strAspect.GetString());
                int modeIndex = SendDlgItemMessage(hDialog, IDC_MODES, CB_ADDSTRING, 0, (WPARAM)modeString.GetString());
                if (mode.width == width && mode.height == height)
                {
                    selectIndex = modeIndex;
                }
            }

            SendDlgItemMessage(hDialog, IDC_FULLSCREEN, BM_SETCHECK, windowed ? BST_UNCHECKED : BST_CHECKED, 0);

            SendDlgItemMessage(hDialog, IDC_MODES, CB_SETMINVISIBLE, 5, 0);
            SendDlgItemMessage(hDialog, IDC_MODES, CB_SETEXTENDEDUI, TRUE, 0);
            SendDlgItemMessage(hDialog, IDC_MODES, CB_SETCURSEL, selectIndex, 0);
            return TRUE;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                std::vector<Gek::Display::Mode> modeList = Gek::Display::getModes()[32];
                UINT32 selectIndex = SendDlgItemMessage(hDialog, IDC_MODES, CB_GETCURSEL, 0, 0);
                auto &mode = modeList[selectIndex];

                Gek::Xml::Document xmlDocument;
                xmlDocument.load(L"%root%\\config.xml");
                Gek::Xml::Node xmlConfigNode = xmlDocument.getRoot();
                if (!xmlConfigNode || xmlConfigNode.getType().CompareNoCase(L"config") != 0)
                {
                    xmlDocument.create(L"config");
                    xmlConfigNode = xmlDocument.getRoot();
                }

                Gek::Xml::Node xmlDisplayNode = xmlConfigNode.firstChildElement(L"display");
                if (!xmlDisplayNode)
                {
                    xmlDisplayNode = xmlConfigNode.createChildElement(L"display");
                }

                xmlDisplayNode.setAttribute(L"width", L"%d", mode.width);
                xmlDisplayNode.setAttribute(L"height", L"%d", mode.height);
                xmlDisplayNode.setAttribute(L"windowed", L"%s", (SendDlgItemMessage(hDialog, IDC_FULLSCREEN, BM_GETCHECK, 0, 0) == BST_UNCHECKED ? L"true" : L"false"));
                xmlDocument.save(L"%root%\\config.xml");

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

int CALLBACK wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR strCommandLine, _In_ int nCmdShow)
{
    if (DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, DialogProc) == IDOK)
    {
        CComPtr<Gek::Context::Interface> context;
        Gek::Context::create(&context);
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
            CComPtr<Gek::Engine::Core::Interface> engineCore;
            context->createInstance(__uuidof(Gek::Engine::Core::Class), IID_PPV_ARGS(&engineCore));
            if (engineCore)
            {
            }
        }

        return 0;
    }

    return -1;
}