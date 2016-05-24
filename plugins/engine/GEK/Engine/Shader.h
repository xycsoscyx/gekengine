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
    GEK_PREDECLARE(RenderPipeline);
    GEK_PREDECLARE(RenderContext);

    GEK_INTERFACE(Shader)
    {
        GEK_PREDECLARE(Block);

        GEK_INTERFACE(Pass)
        {
            enum class Mode : UINT8
            {
                Forward = 0,
                Deferred,
                Compute,
            };

            typedef std::unique_ptr<Pass> Iterator;

            virtual Iterator next(void);
            virtual Mode prepare(void);
        };

        GEK_INTERFACE(Block)
        {
            typedef std::unique_ptr<Block> Iterator;

            virtual Iterator next(void);
            virtual Pass::Iterator begin(void);
            virtual bool prepare(void);
        };

        virtual void initialize(IUnknown *initializerContext, const wchar_t *fileName) = 0;

        virtual UINT32 getPriority(void) = 0;

        virtual void loadResourceList(const wchar_t *materialName, std::unordered_map<CStringW, CStringW> &resourceMap, std::list<ResourceHandle> &resourceList) = 0;
        virtual void setResourceList(RenderContext *renderContext, Block *block, Pass *pass, const std::list<ResourceHandle> &materialMapList) = 0;

        virtual Block::Iterator begin(RenderContext *renderContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum) = 0;
    };
}; // namespace Gek
