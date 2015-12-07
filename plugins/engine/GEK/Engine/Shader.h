#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Utility\XML.h"
#include <functional>

namespace Gek
{
    DECLARE_INTERFACE_IID(Shader, "E4410687-FA71-4177-922D-B8A4C30EDB1D") : virtual public IUnknown
    {
        STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;

        STDMETHOD_(void, loadResourceList)          (THIS_ LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::vector<ResourceHandle> &resourceList) PURE;
        STDMETHOD_(void, setResourceList)           (THIS_ VideoContext *videoContext, const std::vector<ResourceHandle> &materialMapList) PURE;

        STDMETHOD_(void, draw)                      (THIS_ VideoContext *videoContext,
            std::function<void(LPCVOID passData, bool lighting)> drawForward,
            std::function<void(LPCVOID passData, bool lighting)> drawDeferred,
            std::function<void(LPCVOID passData, bool lighting, UINT32 dispatchWidth, UINT32 dispatchHeight, UINT32 dispatchDepth)> drawCompute) PURE;
    };

    DECLARE_INTERFACE_IID(ShaderRegistration, "02B8870C-2AEC-48FD-8F47-34166C9F16C6");
}; // namespace Gek
