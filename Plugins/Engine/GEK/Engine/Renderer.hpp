/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: c85d467e8da7e856f4a8229e8bfbcc46299d722e $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Tue Oct 25 14:51:24 2016 +0000 $
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Shapes/Frustum.hpp"
#include <nano_signal_slot.hpp>

namespace Gek
{
	namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        GEK_INTERFACE(Renderer)
        {
            struct DirectionalLightData
            {
                Math::Float3 radiance;
				float padding1;
				Math::Float3 direction;
				float padding2;
			};

            struct PointLightData
            {
                Math::Float3 radiance;
				float radius;
				Math::Float3 position;
                float range;
            };

            struct SpotLightData
            {
                Math::Float3 radiance;
				float radius;
				Math::Float3 position;
                float range;
                Math::Float3 direction;
				float padding1;
                float innerAngle;
                float outerAngle;
				float coneFalloff;
				float padding2;
            };

            Nano::Signal<void(const Shapes::Frustum &viewFrustum, Math::Float4x4 const &viewMatrix)> onRenderScene;

            virtual ~Renderer(void) = default;

            virtual void queueDrawCall(VisualHandle plugin, MaterialHandle material, std::function<void(Video::Device::Context *)> &&draw) = 0;
            virtual void queueRenderCall(Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, float nearClip, float farClip, ResourceHandle cameraTarget = ResourceHandle()) = 0;

            virtual void renderOverlay(Video::Device::Context *videoContext, ResourceHandle input, ResourceHandle target) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
