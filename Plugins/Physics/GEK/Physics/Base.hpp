/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK/Math/Vector3.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/API/Entity.hpp"
#include <wink/signal.hpp>
#include <dNewton/ndNewton.h>

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Scene)
        {
			GEK_COMPONENT_DATA(Scene);
        };

        GEK_COMPONENT(Physical)
        {
			GEK_COMPONENT_DATA(Physical);

            float mass = 0.0f;
        };

        GEK_COMPONENT(Player)
        {
			GEK_COMPONENT_DATA(Player);
			
			float height = 6.0f;
            float outerRadius = 1.5f;
            float innerRadius = 0.5f;
            float stairStep = 1.0f;
        };
    };

    namespace Physics
    {
        GEK_INTERFACE(Body)
        {
            virtual ~Body(void) = default;

            virtual ndBody* getAsNewtonBody(void) = 0;
        };

        GEK_INTERFACE(World)
        {
            struct Surface
            {
                bool ghost = false;
                float staticFriction = 0.9f;
                float kineticFriction = 0.5f;
                float elasticity = 0.4f;
                float softness = 1.0f;
            };
            
            wink::signal<wink::slot<void(Plugin::Entity *entity0, Math::Float3 const &position, Math::Float3 const &normal, Plugin::Entity *entity1)>> onCollision;

            virtual ~World(void) = default;

            virtual Math::Float3 getGravity(Math::Float3 const *position = nullptr) = 0;

            virtual uint32_t loadSurface(std::string const &surfaceName) = 0;
            virtual const Surface &getSurface(uint32_t surfaceIndex) const = 0;
        };
    }; // namespace Physics
}; // namespace Gek
