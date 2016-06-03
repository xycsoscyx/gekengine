#pragma once

#include "GEK\Utility\String.h"
#include "GEK\Shapes\Frustum.h"
#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"
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

            virtual Iterator next(void) = 0;
            virtual Mode prepare(void) = 0;
        };

        GEK_INTERFACE(Block)
        {
            typedef std::unique_ptr<Block> Iterator;

            virtual Iterator next(void) = 0;
            virtual Pass::Iterator begin(void) = 0;
            virtual bool prepare(void) = 0;
        };

        virtual UINT32 getPriority(void) = 0;

        virtual void loadResourceList(const wchar_t *materialName, std::unordered_map<String, String> &resourceMap, std::list<ResourceHandle> &resourceList) = 0;
        virtual void setResourceList(RenderContext *renderContext, Block *block, Pass *pass, const std::list<ResourceHandle> &materialMapList) = 0;

        virtual Block::Iterator begin(RenderContext *renderContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum) = 0;
    };
}; // namespace Gek
