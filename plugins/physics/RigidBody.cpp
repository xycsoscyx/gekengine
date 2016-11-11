#include "GEK\Math\Common.hpp"
#include "GEK\Math\SIMD\Matrix4x4.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Newton\Base.hpp"

#include <Newton.h>

namespace Gek
{
    namespace Newton
    {
        class RigidBody
            : public Newton::Entity
        {
        private:
            Newton::World *world;

            Plugin::Entity *entity;
            NewtonBody *newtonBody;

        public:
            RigidBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Plugin::Entity *entity)
                : world(static_cast<Newton::World *>(NewtonWorldGetUserData(newtonWorld)))
                , entity(entity)
                , newtonBody(nullptr)
            {
                GEK_REQUIRE(world);
                GEK_REQUIRE(entity);

				const auto &physical = entity->getComponent<Components::Physical>();
				const auto &transform = entity->getComponent<Components::Transform>();

                Math::SIMD::Float4x4 matrix(transform.getMatrix());
                newtonBody = NewtonCreateDynamicBody(newtonWorld, newtonCollision, matrix.data);
                if (newtonBody == nullptr)
                {
                    throw Newton::UnableToCreateBody("Unable to create rigid body");
                }

                NewtonBodySetUserData(newtonBody, dynamic_cast<Newton::Entity *>(this));
                NewtonBodySetMassProperties(newtonBody, physical.mass, newtonCollision);
            }

            ~RigidBody(void)
            {
                NewtonDestroyBody(newtonBody);
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

            uint32_t getSurface(const Math::Float3 &position, const Math::Float3 &normal)
            {
                return 0;
            }

            void onPreUpdate(int threadHandle)
            {
				const auto &physical = entity->getComponent<Components::Physical>();
				const auto &transform = entity->getComponent<Components::Transform>();

                NewtonCollisionSetScale(NewtonBodyGetCollision(newtonBody), transform.scale.x, transform.scale.y, transform.scale.z);

                Math::Float3 gravity(world->getGravity(transform.position));
                NewtonBodyAddForce(newtonBody, (gravity * physical.mass).data);
            }

            void onSetTransform(const float* const matrixData, int threadHandle)
            {
                Math::SIMD::Float4x4 matrix(matrixData);
				auto &transform = entity->getComponent<Components::Transform>();
				transform.position = matrix.translation;
                transform.rotation = Math::convert(matrix);
            }
        };

        Newton::EntityPtr createRigidBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Plugin::Entity *entity)
        {
            return std::make_shared<RigidBody>(newtonWorld, newtonCollision, entity);
        }
    }; // namespace Newton
}; // namespace Gek