#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Context\Broadcaster.h"
#include "GEK\Engine\Resources.h"
#include "GEK\System\VideoDevice.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    namespace Video
    {
        ElementType getElementType(const wchar_t *elementClassString);
        Format getFormat(const wchar_t *formatString);
        DepthWrite getDepthWriteMask(const wchar_t *depthWrite);
        ComparisonFunction getComparisonFunction(const wchar_t *comparisonFunction);
        StencilOperation getStencilOperation(const wchar_t *stencilOperation);
        FillMode getFillMode(const wchar_t *fillMode);
        CullMode getCullMode(const wchar_t *cullMode);
        BlendSource getBlendSource(const wchar_t *blendSource);
        BlendOperation getBlendOperation(const wchar_t *blendOperation);
    };

    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        GEK_INTERFACE(RendererListener)
        {
            virtual void onRenderBackground(void) { };
            virtual void onRenderScene(Plugin::Entity *cameraEntity, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum) { };
            virtual void onRenderForeground(void) { };
        };

        GEK_INTERFACE(Renderer)
            : public Broadcaster<RendererListener>
        {
            GEK_START_EXCEPTIONS();

        virtual Video::Device * getDevice(void) const = 0;

        virtual void render(Plugin::Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float nearClip, float farClip, ResourceHandle cameraTarget) = 0;
        virtual void queueDrawCall(VisualHandle plugin, MaterialHandle material, std::function<void(Video::Device::Context *)> draw) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
