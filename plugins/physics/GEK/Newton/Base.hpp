#pragma once

#include "GEK\Math\Float3.hpp"
#include "GEK\Engine\Component.hpp"
#include "GEK\Engine\Entity.hpp"
#include <nano_signal_slot.hpp>
#include <Newton.h>

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Physical)
        {
            float mass;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };

        GEK_COMPONENT(Player)
        {
			float height;
            float outerRadius;
            float innerRadius;
            float stairStep;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };
    };

    namespace Newton
    {
        GEK_START_EXCEPTIONS();
        GEK_ADD_EXCEPTION(UnableToCreateCollision);
        GEK_ADD_EXCEPTION(InvalidModelIdentifier);
        GEK_ADD_EXCEPTION(InvalidModelType);
        GEK_ADD_EXCEPTION(InvalidModelVersion);

        GEK_INTERFACE(Entity)
        {
            virtual Plugin::Entity * const getEntity(void) const = 0;

            virtual NewtonBody * const getNewtonBody(void) const = 0;

            virtual uint32_t getSurface(const Math::Float3 &position, const Math::Float3 &normal) = 0;

            // Called before the update phase to set the frame data for the body
            // Applies to rigid and player bodies
            virtual void onPreUpdate(float frameTime, int threadHandle) { };

            // Called after the update phase to react to changes in the world
            // Applies to player bodies only
            virtual void onPostUpdate(float frameTime, int threadHandle) { };

            // Called when setting the transformation matrix of the body
            // Applies to rigid bodies only
            virtual void onSetTransform(const float* const matrixData, int threadHandle) { };
        };

        GEK_INTERFACE(World)
        {
            struct Surface
            {
                bool ghost;
                float staticFriction;
                float kineticFriction;
                float elasticity;
                float softness;

                Surface(void)
                    : ghost(false)
                    , staticFriction(0.9f)
                    , kineticFriction(0.5f)
                    , elasticity(0.4f)
                    , softness(1.0f)
                {
                }
            };
            
            Nano::Signal<void(Plugin::Entity *entity0, Plugin::Entity *entity1, const Math::Float3 &position, const Math::Float3 &normal)> onCollision;

            virtual Math::Float3 getGravity(const Math::Float3 &position) = 0;

            virtual uint32_t loadSurface(const wchar_t *fileName) = 0;
            virtual const Surface &getSurface(uint32_t surfaceIndex) const = 0;
        };
    };
}; // namespace Gek
