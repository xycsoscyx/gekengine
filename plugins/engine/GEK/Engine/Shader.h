#pragma once

#include "GEK\Shapes\Frustum.h"
#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Utility\XML.h"
#include <functional>
#include <memory>

namespace Gek
{
    DECLARE_INTERFACE(RenderPipeline);
    DECLARE_INTERFACE(RenderContext);

    DECLARE_INTERFACE_IID(Shader, "E4410687-FA71-4177-922D-B8A4C30EDB1D") : virtual public IUnknown
    {
        DECLARE_INTERFACE(Block);

        DECLARE_INTERFACE(Pass)
        {
            enum class Mode : UINT8
            {
                Forward = 0,
                Deferred,
                Compute,
            };

            typedef std::unique_ptr<Pass> Iterator;

            STDMETHOD_(Iterator, next)              (THIS) PURE;
            STDMETHOD_(Mode, prepare)               (THIS) PURE;
        };

        DECLARE_INTERFACE(Block)
        {
            typedef std::unique_ptr<Block> Iterator;

            STDMETHOD_(Iterator, next)              (THIS) PURE;
            STDMETHOD_(Pass::Iterator, begin)       (THIS) PURE;
            STDMETHOD_(bool, prepare)               (THIS) PURE;
        };

        STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;

        STDMETHOD_(void, loadResourceList)          (THIS_ LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::list<ResourceHandle> &resourceList) PURE;
        STDMETHOD_(void, setResourceList)           (THIS_ RenderContext *renderContext, Block *block, Pass *pass, const std::list<ResourceHandle> &materialMapList) PURE;

        STDMETHOD_(Block::Iterator, begin)          (THIS_ RenderContext *renderContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum) PURE;
    };

    DECLARE_INTERFACE_IID(ShaderRegistration, "02B8870C-2AEC-48FD-8F47-34166C9F16C6");
}; // namespace Gek
