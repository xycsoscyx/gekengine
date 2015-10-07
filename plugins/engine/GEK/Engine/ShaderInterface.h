#pragma once

#include "GEK\Context\ObserverInterface.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Shape\Frustum.h"
#include "GEK\Utility\XML.h"
#include <functional>

namespace Gek
{
    namespace Engine
    {
        namespace Render
        {
            namespace Shader
            {
                DECLARE_INTERFACE_IID(Class, "02B8870C-2AEC-48FD-8F47-34166C9F16C6");

                DECLARE_INTERFACE_IID(Interface, "E4410687-FA71-4177-922D-B8A4C30EDB1D") : virtual public IUnknown
                {
                    STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;

                    STDMETHOD(getMaterialValues)                (THIS_ LPCWSTR fileName, Gek::Xml::Node &xmlMaterialNode, std::vector<CComPtr<Video::Texture::Interface>> &materialMapList, std::vector<UINT32> &materialPropertyList) PURE;
                    STDMETHOD_(void, setMaterialValues)         (THIS_ Video::Context::Interface *context, LPCVOID passData, const std::vector<CComPtr<Video::Texture::Interface>> &materialMapList, const std::vector<UINT32> &materialPropertyList) PURE;

                    STDMETHOD_(void, draw)                      (THIS_ Video::Context::Interface *context,
                        std::function<void(LPCVOID passData, bool lighting)> drawForward,
                        std::function<void(LPCVOID passData, bool lighting)> drawDeferred,
                        std::function<void(LPCVOID passData, bool lighting)> drawCompute) PURE;
                };
            }; // namespace Shader
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
