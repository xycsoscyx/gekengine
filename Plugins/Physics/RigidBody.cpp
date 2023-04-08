#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Physics/Base.hpp"

namespace Gek
{
    namespace Physics
    {
        class RigidBody
            : public Body
            , public ndBodyDynamic
        {
        private:
			class NotifyCallback
				: public ndBodyNotify
			{
            private:
                RigidBody *rigidBody = nullptr;
            
            public:
                NotifyCallback(World* world, RigidBody* rigidBody)
                    : ndBodyNotify(world->getGravity().data)
                    , rigidBody(rigidBody)
                {
                }

                ~NotifyCallback()
                {
                }

                // ndBodyNotify
				void* GetUserData() const
				{
					return rigidBody;
				}

                void OnObjectPick() const
                {
                    rigidBody->OnObjectPick();
                }

                void OnTransform(ndInt32 threadIndex, const ndMatrix& matrix)
                {
                    rigidBody->OnTransform(threadIndex, matrix);
                }

                void OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timeStep)
                {
                    rigidBody->OnApplyExternalForce(threadIndex, timeStep);
                }
			};

			World *world = nullptr;
            Plugin::Entity * const entity = nullptr;

        public:
            RigidBody(World* world, Plugin::Entity* const entity)
                : world(world)
                , entity(entity)
            {
                assert(entity);

                SetNotifyCallback(new NotifyCallback(world, this));
            }

            ~RigidBody(void)
            {
            }

            // Body
            ndBody* getAsNewtonBody(void)
            {
                return GetAsBody();
            }

            // NotifyCallback
            void OnObjectPick() const
            {
            }

            void OnTransform(ndInt32 threadIndex, const ndMatrix& matrixData)
            {
                auto matrix = reinterpret_cast<const Math::Float4x4*>(&matrixData);
                auto& transformComponent = entity->getComponent<Components::Transform>();
                transformComponent.rotation = matrix->getRotation();
                transformComponent.position = matrix->translation.xyz;
            }

            void OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timestep)
            {
                auto const& physicalComponent = entity->getComponent<Components::Physical>();
                auto const& transformComponent = entity->getComponent<Components::Transform>();

                Math::Float3 gravity(world->getGravity(&transformComponent.position));
                SetForce((gravity * physicalComponent.mass).data);
                SetTorque(Math::Float3::Zero.data);
            }
        };

        BodyPtr createRigidBody(World *world, Plugin::Entity * const entity)
        {
            return std::make_unique<RigidBody>(world, entity);
        }
    }; // namespace Physics
}; // namespace Gek