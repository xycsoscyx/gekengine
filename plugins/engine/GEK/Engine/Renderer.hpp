#pragma once

#include "GEK\Context\Context.hpp"
#include "GEK\Context\Broadcaster.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Shapes\Frustum.hpp"

namespace Gek
{
	namespace Utility
	{
		Video::Format getFormat(const wchar_t *formatString);
		Video::DepthStateInformation::Write getDepthWriteMask(const wchar_t *depthWrite);
		Video::ComparisonFunction getComparisonFunction(const wchar_t *comparisonFunction);
		Video::DepthStateInformation::StencilStateInformation::Operation getStencilOperation(const wchar_t *stencilOperation);
		Video::RenderStateInformation::FillMode getFillMode(const wchar_t *fillMode);
		Video::RenderStateInformation::CullMode getCullMode(const wchar_t *cullMode);
		Video::BlendStateInformation::Source getBlendSource(const wchar_t *blendSource);
		Video::BlendStateInformation::Operation getBlendOperation(const wchar_t *blendOperation);
		Video::InputElement::Source getElementSource(const wchar_t *elementClassString);
		Video::InputElement::Semantic getElementSemantic(const wchar_t *semantic);
		//virtual String getElementSemantic(InputElement::Semantic semantic) = 0;
	}; // namespace Utility
	
	namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        GEK_INTERFACE(RendererListener)
        {
            virtual void onRenderScene(const Plugin::Entity *cameraEntity, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum) { };
            virtual void onRenderDisplay(void) { };
        };

        GEK_INTERFACE(Renderer)
            : public Broadcaster<RendererListener>
        {
            GEK_START_EXCEPTIONS();

        virtual Video::Device * getDevice(void) const = 0;

        virtual void render(const Plugin::Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float nearClip, float farClip, ResourceHandle cameraTarget) = 0;
        virtual void queueDrawCall(VisualHandle plugin, MaterialHandle material, std::function<void(Video::Device::Context *)> draw) = 0;
        };
    }; // namespace Engine
}; // namespace Gek