#include <initguid.h>
#include <cguid.h>

#include "GEKContext.h"

#include "GEKAPICLSIDs.h"
#include "GEKComponentsCLSIDs.h"

// {25123D25-1D69-433C-BBB1-224848813F7D}
DEFINE_GUID(CLSID_GEKComponentSystemTransform, 0x25123d25, 0x1d69, 0x433c, 0xbb, 0xb1, 0x22, 0x48, 0x48, 0x81, 0x3f, 0x7d);

// {FA96A45C-68F4-4F24-BEC7-D48A32EEA7F5}
DEFINE_GUID(CLSID_GEKComponentSystemNewton, 0xfa96a45c, 0x68f4, 0x4f24, 0xbe, 0xc7, 0xd4, 0x8a, 0x32, 0xee, 0xa7, 0xf5);

// {1CE9F11B-F995-4595-B541-83E92119974E}
DEFINE_GUID(CLSID_GEKComponentSystemStaticMesh, 0x1ce9f11b, 0xf995, 0x4595, 0xb5, 0x41, 0x83, 0xe9, 0x21, 0x19, 0x97, 0x4e);

// {9DED5A4D-433E-4D66-ADB6-51FE0ABF09FA}
DEFINE_GUID(CLSID_GEKComponentSystemLight, 0x9ded5a4d, 0x433e, 0x4d66, 0xad, 0xb6, 0x51, 0xfe, 0xa, 0xbf, 0x9, 0xfa);

// {CA7DB8F4-91D2-47E4-BE98-488E51092076}
DEFINE_GUID(CLSID_GEKComponentSystemViewer, 0xca7db8f4, 0x91d2, 0x47e4, 0xbe, 0x98, 0x48, 0x8e, 0x51, 0x9, 0x20, 0x76);

// {92336C35-CE7C-4B43-8DD7-0C6783F27E0B}
DEFINE_GUID(CLSID_GEKComponentSystemScript, 0x92336c35, 0xce7c, 0x4b43, 0x8d, 0xd7, 0xc, 0x67, 0x83, 0xf2, 0x7e, 0xb);

// {EA35B983-E069-4207-AF88-74678C3F8C69}
DEFINE_GUID(CLSID_GEKComponentSystemLogic, 0xea35b983, 0xe069, 0x4207, 0xaf, 0x88, 0x74, 0x67, 0x8c, 0x3f, 0x8c, 0x69);

DECLARE_REGISTERED_CLASS(CGEKComponentSystemTransform)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemNewton)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemModel)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemLight)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemViewer)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemScript)
DECLARE_REGISTERED_CLASS(CGEKComponentSystemLogic)

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemTransform, CGEKComponentSystemTransform)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemNewton, CGEKComponentSystemNewton)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemStaticMesh, CGEKComponentSystemModel)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemLight, CGEKComponentSystemLight)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemViewer, CGEKComponentSystemViewer)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemScript, CGEKComponentSystemScript)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
    ADD_CONTEXT_CLASS(CLSID_GEKComponentSystemLogic, CGEKComponentSystemLogic)
        ADD_CLASS_TYPE(CLSID_GEKComponentSystemType)
END_CONTEXT_SOURCE