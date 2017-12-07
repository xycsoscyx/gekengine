#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Newton/Base.hpp"

#include <Newton.h>

namespace Gek
{
    namespace Newton
    {
        class RigidBody
            : public Newton::Entity
        {
        private:
            Newton::World *world = nullptr;
            Plugin::Entity * const entity = nullptr;
            NewtonBody *newtonBody = nullptr;

        public:
            RigidBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Plugin::Entity * const entity)
                : world(static_cast<Newton::World *>(NewtonWorldGetUserData(newtonWorld)))
                , entity(entity)
            {
                assert(world);
                assert(entity);

				auto const &physicalComponent = entity->getComponent<Components::Physical>();
				auto const &transformComponent = entity->getComponent<Components::Transform>();

                Math::Float4x4 matrix(transformComponent.getMatrix());
                newtonBody = NewtonCreateDynamicBody(newtonWorld, newtonCollision, matrix.data);
				assert(newtonBody && "Unable to create rigid body");

                NewtonBodySetCollisionScale(newtonBody, transformComponent.scale.x, transformComponent.scale.y, transformComponent.scale.z);
                NewtonBodySetUserData(newtonBody, dynamic_cast<Newton::Entity *>(this));
                NewtonBodySetMassProperties(newtonBody, physicalComponent.mass, newtonCollision);
                NewtonBodySetCollidable(newtonBody, true);
                NewtonBodySetAutoSleep(newtonBody, true);
            }

            ~RigidBody(void)
            {
            }

            // Newton::Entity
            Plugin::Entity * const getEntity(void) const
            {
                return entity;
            }

            NewtonBody * const getNewtonBody(void) const
            {
                return newtonBody;
            }

            uint32_t getSurface(Math::Float3 const &position, Math::Float3 const &normal)
            {
                return 0;
            }

            void onPreUpdate(float frameTime, int threadHandle)
            {
				auto const &physicalComponent = entity->getComponent<Components::Physical>();
				auto const &transformComponent = entity->getComponent<Components::Transform>();

                Math::Float3 gravity(world->getGravity(transformComponent.position));
                NewtonBodyAddForce(newtonBody, (gravity * physicalComponent.mass).data);
            }

            void onSetTransform(const float* const matrixData, int threadHandle)
            {
                Math::Float4x4 matrix(matrixData);
				auto &transformComponent = entity->getComponent<Components::Transform>();
                transformComponent.rotation = matrix.getRotation();
                transformComponent.position = matrix.translation.xyz;
            }
        };

        Newton::EntityPtr createRigidBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Plugin::Entity * const entity)
        {
            return std::make_unique<RigidBody>(newtonWorld, newtonCollision, entity);
        }
    }; // namespace Newton
}; // namespace Gek