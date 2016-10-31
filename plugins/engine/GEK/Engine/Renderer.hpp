/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: c85d467e8da7e856f4a8229e8bfbcc46299d722e $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Tue Oct 25 14:51:24 2016 +0000 $
#pragma once

#include "GEK\Utility\Context.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Shapes\Frustum.hpp"
#include <nano_signal_slot.hpp>

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

        GEK_INTERFACE(Renderer)
        {
            GEK_START_EXCEPTIONS();

            struct DirectionalLightData
            {
                Math::Float3 color;
				float buffer1;
				Math::Float3 direction;
				float buffer2;
			};

            struct PointLightData
            {
                Math::Float3 color;
				float radius;
				Math::Float3 position;
                float range;
            };

            struct SpotLightData
            {
                Math::Float3 color;
				float radius;
				Math::Float3 position;
                float range;
                Math::Float3 direction;
				float buffer1;
                float innerAngle;
                float outerAngle;
				float coneFalloff;
				float buffer2;
            };

            Nano::Signal<void(const Shapes::Frustum &viewFrustum, const Math::SIMD::Float4x4 &viewMatrix)> onRenderScene;

            virtual Video::Device * getVideoDevice(void) const = 0;

            virtual void renderOverlay(Video::Device::Context *videoContext, ResourceHandle input, ResourceHandle target) = 0;
            virtual void render(const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, float nearClip, float farClip, const std::vector<String> *filterList = nullptr, ResourceHandle cameraTarget = ResourceHandle()) = 0;
            virtual void queueDrawCall(VisualHandle plugin, MaterialHandle material, std::function<void(Video::Device::Context *)> draw) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
