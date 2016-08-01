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
        ElementType getElementType(const String &elementClassString);
        Format getFormat(const String &formatString);
        DepthWrite getDepthWriteMask(const String &depthWrite);
        ComparisonFunction getComparisonFunction(const String &comparisonFunction);
        StencilOperation getStencilOperation(const String &stencilOperation);
        FillMode getFillMode(const String &fillMode);
        CullMode getCullMode(const String &cullMode);
        BlendSource getBlendSource(const String &blendSource);
        BlendOperation getBlendOperation(const String &blendOperation);
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
