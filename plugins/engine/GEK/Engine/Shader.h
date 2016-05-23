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
    interface RenderPipeline;
    interface RenderContext;

    interface Shader
    {
        interface Block;

        interface Pass
        {
            enum class Mode : UINT8
            {
                Forward = 0,
                Deferred,
                Compute,
            };

            typedef std::unique_ptr<Pass> Iterator;

            Iterator next(void);
            Mode prepare(void);
        };

        interface Block
        {
            typedef std::unique_ptr<Block> Iterator;

            Iterator next(void);
            Pass::Iterator begin(void);
            bool prepare(void);
        };

        void initialize(IUnknown *initializerContext, LPCWSTR fileName);

        UINT32 getPriority(void);

        void loadResourceList(LPCWSTR materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::list<ResourceHandle> &resourceList);
        void setResourceList(RenderContext *renderContext, Block *block, Pass *pass, const std::list<ResourceHandle> &materialMapList);

        Block::Iterator begin(RenderContext *renderContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum);
    };

    DECLARE_INTERFACE_IID(ShaderRegistration, "02B8870C-2AEC-48FD-8F47-34166C9F16C6");
}; // namespace Gek
