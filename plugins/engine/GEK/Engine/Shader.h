#pragma once

#include "GEK\Shapes\Frustum.h"
#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Utility\XML.h"
#include <functional>

namespace Gek
{
    DECLARE_INTERFACE(RenderPipeline);
    DECLARE_INTERFACE(RenderContext);

    DECLARE_INTERFACE_IID(Shader, "E4410687-FA71-4177-922D-B8A4C30EDB1D") : virtual public IUnknown
    {
        enum class PassMode : UINT8
        {
            Forward = 0,
            Deferred,
            Compute,
        };

        struct Pass
        {
            LPVOID data;

            Pass(LPVOID data);

            Pass next(void);

            PassMode prepare(RenderContext *renderContext);
        };

        struct Block
        {
            LPVOID data;

            Block(LPVOID data);

            Block next(void);

            Pass begin(void);

            void prepare(RenderContext *renderContext, const Shapes::Frustum &viewFrustum);
        };

        STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;

        STDMETHOD_(void, loadResourceList)          (THIS_ LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::list<ResourceHandle> &resourceList) PURE;
        STDMETHOD_(void, setResourceList)           (THIS_ RenderContext *renderContext, Block *block, Pass *pass, const std::list<ResourceHandle> &materialMapList) PURE;

        STDMETHOD_(Block, begin)                    (THIS) PURE;
    };

    DECLARE_INTERFACE_IID(ShaderRegistration, "02B8870C-2AEC-48FD-8F47-34166C9F16C6");
}; // namespace Gek
