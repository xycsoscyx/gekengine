#include "GEK\Math\Common.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Newton\Base.hpp"
#include <algorithm>
#include <memory>

#include <Newton.h>

namespace Gek
{
	namespace Newton
	{
        static const int PLAYER_CONTROLLER_MAX_CONTACTS = 32;
        static const float PLAYER_MIN_RESTRAINING_DISTANCE = 1.0e-2f;

        static const int D_DESCRETE_MOTION_STEPS = 8;
        static const int D_PLAYER_MAX_INTERGRATION_STEPS = 8;
        static const int D_PLAYER_MAX_SOLVER_ITERATIONS = 16;
        static const float D_PLAYER_CONTACT_SKIN_THICKNESS = 0.025f;

        class ConvexCastPreFilter
        {
        protected:
            const NewtonBody *newtonBody;

        public:
            ConvexCastPreFilter(void)
                : newtonBody(nullptr)
            {
            }

            ConvexCastPreFilter(const NewtonBody* const newtonBody)
                :newtonBody(newtonBody)
            {
            }

            virtual unsigned preFilter(const NewtonBody* const newtonBody, const NewtonCollision* const myCollision)
            {
                return NewtonCollisionGetMode(myCollision);
            }

            static unsigned preFilter(const NewtonBody* const newtonBody, const NewtonCollision* const myCollision, void* const userData)
            {
                ConvexCastPreFilter* const filter = static_cast<ConvexCastPreFilter *>(userData);
                return (newtonBody != filter->newtonBody) ? filter->preFilter(newtonBody, myCollision) : 0;
            }
        };

		class PlayerBody;

		GEK_INTERFACE(State)
		{
			virtual void onEnter(PlayerBody *player) { };
			virtual void onExit(PlayerBody *player) { };

			virtual StatePtr onUpdate(PlayerBody *player, float frameTime) { return nullptr; };
			virtual StatePtr onAction(PlayerBody *player, const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter) { return nullptr; };
		};

		class IdleState
			: public State
		{
		public:
			StatePtr onUpdate(PlayerBody *player, float frameTime);
			StatePtr onAction(PlayerBody *player, const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter);
		};

		class CrouchingState
			: public State
		{
		public:
			StatePtr onUpdate(PlayerBody *player, float frameTime);
			StatePtr onAction(PlayerBody *player, const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter);
		};

		class WalkingState
			: public State
		{
		public:
			StatePtr onUpdate(PlayerBody *player, float frameTime);
			StatePtr onAction(PlayerBody *player, const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter);
		};

		class JumpingState
			: public State
		{
		public:
			void onEnter(PlayerBody *player);
			void onExit(PlayerBody *player);
			StatePtr onUpdate(PlayerBody *player, float frameTime);
		};

		class FallingState
			: public State
		{
		private:
			float time;

		public:
			void onEnter(PlayerBody *player);
			StatePtr onUpdate(PlayerBody *player, float frameTime);
		};

		class DroppingState
			: public State
		{
		public:
			StatePtr onUpdate(PlayerBody *player, float frameTime);
		};

		class PlayerBody
			: public Newton::Entity
		{
		public:
            Plugin::Population *population = nullptr;
			Newton::World *world = nullptr;
			NewtonWorld *newtonWorld = nullptr;

            Plugin::Entity *entity = nullptr;
			float maximumSlope = 0.0f;
			float restrainingDistance = 0.0f;
			float sphereCastOrigin = 0.0f;

			NewtonBody *newtonBody = nullptr;
			NewtonCollision *newtonCastingShape = nullptr;
			NewtonCollision *newtonSupportShape = nullptr;
			NewtonCollision *newtonUpperBodyShape = nullptr;

			StatePtr currentState;
            bool moveForward = false;
            bool moveBackward = false;
            bool strafeLeft = false;
            bool strafeRight = false;
			float headingAngle = 0.0f;
			float forwardSpeed = 0.0f;
			float lateralSpeed = 0.0f;
			float verticalSpeed = 0.0f;

            Math::Float3 groundNormal = Math::Float3::Zero;
            Math::Float3 groundVelocity = Math::Float3::Zero;

            bool jumping = false;
            bool crouching = false;
            bool touchingSurface = false;
            bool falling = true;

		public:
			PlayerBody(Plugin::Population *population,
				NewtonWorld *newtonWorld,
				Plugin::Entity *entity)
				: population(population)
				, world(static_cast<Newton::World *>(NewtonWorldGetUserData(newtonWorld)))
				, newtonWorld(newtonWorld)
				, entity(entity)
				, currentState(std::make_shared<IdleState>())
			{
				const auto &physicalComponent = entity->getComponent<Components::Physical>();
				const auto &transformComponent = entity->getComponent<Components::Transform>();
				const auto &playerComponent = entity->getComponent<Components::Player>();
                float halfHeight = (playerComponent.height * 0.5f);

                setRestrainingDistance(0.0f);
                setClimbSlope(Math::convertDegreesToRadians(45.0f));

                const int stepCount = 12;
                Math::Float3 convexPoints[2][stepCount];

                // create an inner thin cylinder
                Math::Float3 p0(playerComponent.innerRadius, 0.0f, 0.0f);
                Math::Float3 p1(playerComponent.innerRadius, halfHeight, 0.0f);
                for (int step = 0; step < stepCount; step++)
                {
                    Math::SIMD::Float4x4 rotation(Math::SIMD::Float4x4::createYawRotation(step * 2.0f * Math::Pi / stepCount));
                    convexPoints[0][step] = rotation.rotate(p0);
                    convexPoints[1][step] = rotation.rotate(p1);
                }

                NewtonCollision* const supportShape = NewtonCreateConvexHull(newtonWorld, stepCount * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, nullptr);

                // create the outer thick cylinder
                Math::SIMD::Float4x4 outerShapeMatrix(Math::SIMD::Float4x4::Identity);
                float capsuleHigh = halfHeight - playerComponent.stairStep;
                outerShapeMatrix.translation = outerShapeMatrix.ny * (capsuleHigh * 0.5f + playerComponent.stairStep);
                outerShapeMatrix.tw = 1.0f;

                NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.25f, 0.5f, 0, outerShapeMatrix.data);
                NewtonCollisionSetScale(bodyCapsule, playerComponent.outerRadius * 4.0f, capsuleHigh, playerComponent.outerRadius * 4.0f);

                // compound collision player controller
                NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
                NewtonCompoundCollisionBeginAddRemove(playerShape);
                NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
                NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
                NewtonCompoundCollisionEndAddRemove(playerShape);

                // create the kinematic body
                newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, Math::SIMD::Float4x4::Identity.data);
                NewtonBodySetUserData(newtonBody, (Newton::Entity *)this);

                // players must have weight, otherwise they are infinitely strong when they collide
                NewtonCollision* const shape = NewtonBodyGetCollision(newtonBody);
                NewtonBodySetMassProperties(newtonBody, physicalComponent.mass, shape);

                // make the body collidable with other dynamics bodies, by default
                NewtonBodySetCollidable(newtonBody, true);

                float castHeight = capsuleHigh * 0.4f;
                float castRadius = (playerComponent.innerRadius * 0.5f > 0.05f) ? playerComponent.innerRadius * 0.5f : 0.05f;
                Math::Float3 q0(castRadius, 0.0f, 0.0f);
                Math::Float3 q1(castRadius, castHeight, 0.0f);
                for (int step = 0; step < stepCount; step++)
                {
                    Math::SIMD::Float4x4 rotation(Math::SIMD::Float4x4::createYawRotation(step * 2.0f * Math::Pi / stepCount));
                    convexPoints[0][step] = rotation.rotate(q0);
                    convexPoints[1][step] = rotation.rotate(q1);
                }

                newtonCastingShape = NewtonCreateConvexHull(newtonWorld, stepCount * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, nullptr);
                newtonSupportShape = NewtonCompoundCollisionGetCollisionFromNode(shape, NewtonCompoundCollisionGetNodeByIndex(shape, 0));
                newtonUpperBodyShape = NewtonCompoundCollisionGetCollisionFromNode(shape, NewtonCompoundCollisionGetNodeByIndex(shape, 1));

                NewtonDestroyCollision(bodyCapsule);
                NewtonDestroyCollision(supportShape);
                NewtonDestroyCollision(playerShape);
            }

			~PlayerBody(void)
			{
				NewtonDestroyCollision(newtonCastingShape);
                population->onAction.disconnect<PlayerBody, &PlayerBody::onAction>(this);
			}

			// Plugin::Population Slots
			void onAction(const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter)
			{
				if (_wcsicmp(actionName, L"turn") == 0)
				{
					headingAngle += (parameter.value * 0.01f);
				}
				else if (_wcsicmp(actionName, L"move_forward") == 0)
				{
					moveForward = parameter.state;
				}
				else if (_wcsicmp(actionName, L"move_backward") == 0)
				{
					moveBackward = parameter.state;
				}
				else if (_wcsicmp(actionName, L"strafe_left") == 0)
				{
					strafeLeft = parameter.state;
				}
				else if (_wcsicmp(actionName, L"strafe_right") == 0)
				{
					strafeRight = parameter.state;
				}
				else if (_wcsicmp(actionName, L"crouch") == 0)
				{
					crouching = parameter.state;
				}

				StatePtr nextState(currentState->onAction(this, actionName, parameter));
				if (nextState)
				{
					currentState->onExit(this);
					nextState->onEnter(this);
					currentState = nextState;
				}
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

            Math::Float3 calculateDesiredOmega(float headingAngle, float frameTime) const
            {
                float newtonRotation[4];
                NewtonBodyGetRotation(newtonBody, newtonRotation);
                Math::Quaternion playerRotation(newtonRotation[1], newtonRotation[2], newtonRotation[3], newtonRotation[0]);

                static const Math::Float3 UpDirection(0.0f, 1.0f, 0.0f);
                Math::Quaternion targetRotation(Math::Quaternion::createAngularRotation(UpDirection, headingAngle));
                return playerRotation.calculateAverageOmega(targetRotation, 0.5f / frameTime);
            }
            
            Math::Float3 calculateDesiredVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed, const Math::Float3& gravity, float frameTime) const
            {
                Math::SIMD::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);

                Math::Float3 velocity;
                if ((verticalSpeed <= 0.0f) && (groundNormal.dot(groundNormal)) > 0.0f)
                {
                    // plane is supported by a ground plane, apply the player input velocity
                    if (groundNormal.dot(matrix.ny) >= maximumSlope)
                    {
                        // player is in a legal slope, he is in full control of his movement
                        Math::Float3 currentVelocity;
                        NewtonBodyGetVelocity(newtonBody, currentVelocity.data);
                        velocity = matrix.ny * currentVelocity.dot(matrix.ny) + (gravity * frameTime) + (matrix.nz * forwardSpeed) + (matrix.nx * lateralSpeed) + (matrix.ny * verticalSpeed);
                        velocity += (groundVelocity - matrix.ny * matrix.ny.dot(groundVelocity));

                        float speedLimit = (forwardSpeed * forwardSpeed) + (lateralSpeed * lateralSpeed) + (verticalSpeed * verticalSpeed) + groundVelocity.dot(groundVelocity) + 0.1f;
                        float speedMagnitude = velocity.dot(velocity);
                        if (speedMagnitude > speedLimit)
                        {
                            velocity = velocity * std::sqrt(speedLimit / speedMagnitude);
                        }
                    }
                    else
                    {
                        // player is in an illegal ramp, he slides down hill an loses control of his movement 
                        NewtonBodyGetVelocity(newtonBody, velocity.data);
                        velocity += matrix.ny * verticalSpeed;
                        velocity += gravity * frameTime;
                    }

                    float velocityAngle = groundNormal.dot(velocity - groundVelocity);
                    if (velocityAngle < 0.0f)
                    {
                        velocity -= groundNormal * velocityAngle;
                    }
                }
                else
                {
                    // player is on free fall, only apply the gravity
                    NewtonBodyGetVelocity(newtonBody, velocity.data);
                    velocity += matrix.ny * verticalSpeed;
                    velocity += gravity * frameTime;
                }

                return velocity;
            }

            void onPreUpdate(float frameTime, int threadHandle)
			{
				StatePtr nextState(currentState->onUpdate(this, frameTime));
				if (nextState)
				{
					currentState->onExit(this);
					nextState->onEnter(this);
					currentState = nextState;
				}

                Math::Float3 omega(calculateDesiredOmega(headingAngle, frameTime));
                NewtonBodySetOmega(newtonBody, omega.data);

                Math::Float3 position;
                NewtonBodyGetPosition(newtonBody, position.data);
                Math::Float3 gravity(world->getGravity(position));

                Math::Float3 velocity(calculateDesiredVelocity(forwardSpeed, lateralSpeed, verticalSpeed, gravity, frameTime));
                NewtonBodySetVelocity(newtonBody, velocity.data);
			}

            void updateGroundPlane(Math::SIMD::Float4x4& matrix, const Math::SIMD::Float4x4& castMatrix, const Math::Float3& distance, int threadHandle)
            {
                NewtonWorldConvexCastReturnInfo information;
                ConvexCastPreFilter filter(newtonBody);

                float castDistance = 10.0f;
                int castCount = NewtonWorldConvexCast(newtonWorld, castMatrix.data, distance.data, newtonCastingShape, &castDistance, &filter, ConvexCastPreFilter::preFilter, &information, 1, threadHandle);

                groundNormal = Math::Float3::Zero;
                groundVelocity = Math::Float3::Zero;
                if (castCount && (castDistance <= 1.0f))
                {
                    touchingSurface = true;
                    Math::Float3 supportPoint(castMatrix.translation + (distance - castMatrix.translation) * castDistance);
                    groundNormal = Math::Float3(information.m_normal);
                    NewtonBodyGetPointVelocity(information.m_hitBody, supportPoint.data, groundVelocity.data);
                    matrix.translation = supportPoint;
                    matrix.tw = 1.0f;
                }
                else
                {
                    touchingSurface = false;
                }
            }
            
            float calculateContactKinematics(const Math::Float3& velocity, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
            {
                Math::Float3 contactVelocity;
                if (contactInfo->m_hitBody)
                {
                    NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, contactVelocity.data);
                }
                else
                {
                    contactVelocity = Math::Float3::Zero;
                }

                const float restitution = 0.0f;
                Math::Float3 normal(contactInfo->m_normal);
                float reboundVelocMag = -(velocity - contactVelocity).dot(normal) * (1.0f + restitution);
                return (reboundVelocMag > 0.0f) ? reboundVelocMag : 0.0f;
            }

            void onPostUpdate(float frameTime, int threadHandle)
			{
                // get the body motion state 
                const auto &playerComponent = entity->getComponent<Components::Player>();
                float halfHeight = (playerComponent.height * 0.5f);

                Math::SIMD::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);

                Math::Float3 velocity;
                NewtonBodyGetVelocity(newtonBody, velocity.data);

                Math::Float3 omega;
                NewtonBodyGetOmega(newtonBody, omega.data);

                // integrate body angular velocity
                float newtonRotation[4];
                NewtonBodyGetRotation(newtonBody, newtonRotation);
                Math::Quaternion bodyRotation(newtonRotation[1], newtonRotation[2], newtonRotation[3], newtonRotation[0]);
                bodyRotation = bodyRotation.integrateOmega(omega, frameTime);
                matrix = Math::convert(bodyRotation, matrix.translation);

                // integrate linear velocity
                float normalizedRemainingTime = 1.0f;
                float descreteTimeStep = frameTime * (1.0f / D_DESCRETE_MOTION_STEPS);
                int previousContactCount = 0;
                ConvexCastPreFilter castFilterData(newtonBody);
                NewtonWorldConvexCastReturnInfo previousContactInformation[PLAYER_CONTROLLER_MAX_CONTACTS];

                Math::Float3 scale;
                NewtonCollisionGetScale(newtonUpperBodyShape, &scale.x, &scale.y, &scale.z);
                const float radius = (playerComponent.outerRadius + restrainingDistance) * 4.0f;
                NewtonCollisionSetScale(newtonUpperBodyShape, radius, halfHeight - playerComponent.stairStep, radius);

                NewtonWorldConvexCastReturnInfo upConstratint;
                memset(&upConstratint, 0, sizeof(upConstratint));
                upConstratint.m_normal[0] = 0.0f;
                upConstratint.m_normal[1] = 1.0f;
                upConstratint.m_normal[2] = 0.0f;
                upConstratint.m_normal[3] = 0.0f;

                for (int integration = 0; (integration < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedRemainingTime > 1.0e-5f); integration++)
                {
                    if (velocity.dot(velocity) < 1.0e-6f)
                    {
                        break;
                    }

                    float timeToImpact;
                    NewtonWorldConvexCastReturnInfo currentContactInformation[PLAYER_CONTROLLER_MAX_CONTACTS];
                    Math::Float3 targetPosition(matrix.translation + (velocity * frameTime));
                    int contactCount = NewtonWorldConvexCast(newtonWorld, matrix.data, targetPosition.data, newtonUpperBodyShape, &timeToImpact, &castFilterData, ConvexCastPreFilter::preFilter , currentContactInformation, sizeof(currentContactInformation) / sizeof(currentContactInformation[0]), threadHandle);
                    if (contactCount)
                    {
                        //contactCount = manager->ProcessContacts(this, currentContactInformation, contactCount);
                    }

                    if (contactCount)
                    {
                        matrix.translation += velocity * (timeToImpact * frameTime);
                        if (timeToImpact > 0.0f)
                        {
                            matrix.translation -= velocity * (D_PLAYER_CONTACT_SKIN_THICKNESS / std::sqrt(velocity.dot(velocity)));
                        }

                        normalizedRemainingTime -= timeToImpact;
                        float speed[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                        float bounceSpeed[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                        Math::Float3 bounceNormal[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                        for (int contact = 1; contact < contactCount; contact++)
                        {
                            Math::Float3 normal0(currentContactInformation[contact - 1].m_normal);
                            for (int cast = 0; cast < contact; cast++)
                            {
                                Math::Float3 normal1(currentContactInformation[cast].m_normal);
                                if (normal0.dot(normal1) > 0.9999f)
                                {
                                    currentContactInformation[contact] = currentContactInformation[contactCount - 1];
                                    contact--;
                                    contactCount--;
                                    break;
                                }
                            }
                        }

                        int castCount = 0;
                        if (touchingSurface)
                        {
                            upConstratint.m_point[0] = matrix.translation.x;
                            upConstratint.m_point[1] = matrix.translation.y;
                            upConstratint.m_point[2] = matrix.translation.z;
                            upConstratint.m_point[3] = 0.0f;

                            speed[castCount] = 0.0f;
                            bounceNormal[castCount] = Math::Float3(upConstratint.m_normal);
                            bounceSpeed[castCount] = calculateContactKinematics(velocity, &upConstratint);
                            castCount++;
                        }

                        for (int contact = 0; contact < contactCount; contact++)
                        {
                            speed[castCount] = 0.0f;
                            bounceNormal[castCount] = Math::Float3(currentContactInformation[contact].m_normal);
                            bounceSpeed[castCount] = calculateContactKinematics(velocity, &currentContactInformation[contact]);
                            castCount++;
                        }

                        for (int contact = 0; contact < previousContactCount; contact++)
                        {
                            speed[castCount] = 0.0f;
                            bounceNormal[castCount] = Math::Float3(previousContactInformation[contact].m_normal);
                            bounceSpeed[castCount] = calculateContactKinematics(velocity, &previousContactInformation[contact]);
                            castCount++;
                        }

                        float redisualVelocity = 10.0f;
                        Math::Float3 auxiliaryBounceVelocity(Math::Float3::Zero);
                        for (int step = 0; (step < D_PLAYER_MAX_SOLVER_ITERATIONS) && (redisualVelocity > 1.0e-3f); step++)
                        {
                            redisualVelocity = 0.0f;
                            for (int cast = 0; cast < castCount; cast++)
                            {
                                Math::Float3 normal(bounceNormal[cast]);
                                float v = bounceSpeed[cast] - normal.dot(auxiliaryBounceVelocity);
                                float x = speed[cast] + v;
                                if (x < 0.0f)
                                {
                                    v = 0.0f;
                                    x = 0.0f;
                                }

                                if (std::abs(v) > redisualVelocity)
                                {
                                    redisualVelocity = std::abs(v);
                                }

                                auxiliaryBounceVelocity += normal * (x - speed[cast]);
                                speed[cast] = x;
                            }
                        }

                        Math::Float3 frameVelocity(Math::Float3::Zero);
                        for (int cast = 0; cast < castCount; cast++)
                        {
                            Math::Float3 normal(bounceNormal[cast]);
                            frameVelocity += normal * speed[cast];
                        }

                        velocity += frameVelocity;
                        float velocityMagnitude = frameVelocity.dot(frameVelocity);
                        if (velocityMagnitude < 1.0e-6f)
                        {
                            float advanceTime = std::min(descreteTimeStep, normalizedRemainingTime * frameTime);
                            matrix.translation += velocity * advanceTime;
                            normalizedRemainingTime -= advanceTime / frameTime;
                        }

                        previousContactCount = contactCount;
                        memcpy(previousContactInformation, currentContactInformation, previousContactCount * sizeof(NewtonWorldConvexCastReturnInfo));
                    }
                    else
                    {
                        matrix.translation = targetPosition;
                        matrix.tw = 1.0f;
                        break;
                    }
                }

                NewtonCollisionSetScale(newtonUpperBodyShape, scale.x, scale.y, scale.z);

                // determine if player is standing on some plane
                Math::SIMD::Float4x4 supportMatrix(matrix);
                supportMatrix.translation += matrix.ny * sphereCastOrigin;
                if (touchingSurface)
                {
                    float step = std::abs(matrix.ny.dot(velocity * frameTime));
                    float castDistance = (groundNormal.dot(groundNormal) > 0.0f) ? playerComponent.stairStep : step;
                    Math::Float3 distance(matrix.translation - matrix.ny * (castDistance * 2.0f));
                    updateGroundPlane(matrix, supportMatrix, distance, threadHandle);
                }
                else
                {
                    Math::Float3 distance(matrix.translation);
                    updateGroundPlane(matrix, supportMatrix, distance, threadHandle);
                }

                // set player velocity, position and orientation
                NewtonBodySetVelocity(newtonBody, velocity.data);
                NewtonBodySetMatrix(newtonBody, matrix.data);

                auto &transformComponent = entity->getComponent<Components::Transform>();
                transformComponent.rotation = Math::convert(matrix);
                transformComponent.position = matrix.translation;
            }

		private:
			void setRestrainingDistance(float distance)
			{
                restrainingDistance = std::max(std::abs (distance), PLAYER_MIN_RESTRAINING_DISTANCE);
            }

			void setClimbSlope(float slopeInRadians)
			{
                maximumSlope = std::cos(std::abs(slopeInRadians));
            }
		};

		/* Idle
		If the ground drops out, the player starts to drop (uncontrolled)
		Actions trigger action states (crouch, walk, jump)
		*/
		StatePtr IdleState::onUpdate(PlayerBody *player, float frameTime)
		{
			if (player->falling)
			{
				return std::make_shared<DroppingState>();
			}

			return nullptr;
		}

		StatePtr IdleState::onAction(PlayerBody *player, const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter)
		{
			if (_wcsicmp(actionName, L"crouch") == 0 && parameter.state)
			{
				return std::make_shared<CrouchingState>();
			}
			else if (_wcsicmp(actionName, L"move_forward") == 0 && parameter.state)
			{
				return std::make_shared<WalkingState>();
			}
			else if (_wcsicmp(actionName, L"move_backward") == 0 && parameter.state)
			{
				return std::make_shared<WalkingState>();
			}
			else if (_wcsicmp(actionName, L"strafe_left") == 0 && parameter.state)
			{
				return std::make_shared<WalkingState>();
			}
			else if (_wcsicmp(actionName, L"strafe_right") == 0 && parameter.state)
			{
				return std::make_shared<WalkingState>();
			}
			else if (_wcsicmp(actionName, L"jump") == 0 && parameter.state)
			{
				return std::make_shared<JumpingState>();
			}

			return nullptr;
		}

		/* Crouch
		Triggered from idle or landing
		If not crouching on update (after jumping or released key), then walks/stands idle
		*/
		StatePtr CrouchingState::onUpdate(PlayerBody *player, float frameTime)
		{
			if (!player->crouching)
			{
				if (player->moveForward || player->moveBackward || player->strafeLeft || player->strafeRight)
				{
					return std::make_shared<WalkingState>();
				}
				else
				{
					return std::make_shared<IdleState>();
				}
			}

			return nullptr;
		}

		StatePtr CrouchingState::onAction(PlayerBody *player, const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter)
		{
			return nullptr;
		}

		/* Walk
		Move when keys pressed
		Jump when jumping
		Idle when no key pressed
		*/
		StatePtr WalkingState::onUpdate(PlayerBody *player, float frameTime)
		{
			if (!player->moveForward && !player->moveBackward && !player->strafeLeft && !player->strafeRight)
			{
				return std::make_shared<IdleState>();
			}

			player->forwardSpeed += (((player->moveForward ? 1.0f : 0.0f) + (player->moveBackward ? -1.0f : 0.0f)) * 5.0f);
			player->lateralSpeed += (((player->strafeLeft ? -1.0f : 0.0f) + (player->strafeRight ? 1.0f : 0.0f)) * 5.0f);
			return nullptr;
		}

		StatePtr WalkingState::onAction(PlayerBody *player, const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter)
		{
			if (_wcsicmp(actionName, L"jump") == 0 && parameter.state)
			{
				return std::make_shared<JumpingState>();
			}

			return nullptr;
		}

		/* Jump
		Holds open jump state while active
		Applies initial vertical velocity for starting push
		If landing, land with a clean crouch (allows for walking from landing)
		Once descent starts, transition to falling state
		*/
		void JumpingState::onEnter(PlayerBody *player)
		{
			player->verticalSpeed += 10.0f;
			player->jumping = true;

			/*
			Break the connection when first jumping:
			touching == true
			Action Update
			jumping = true
			Physics Update
			if touching then crouch
			Without breaking the initial connection, jumps will always instantly switch to crouch
			*/
			player->touchingSurface = false;
		}

		void JumpingState::onExit(PlayerBody *player)
		{
			player->jumping = false;
		}

		StatePtr JumpingState::onUpdate(PlayerBody *player, float frameTime)
		{
			GEK_REQUIRE(player);

			if (player->touchingSurface)
			{
				return std::make_shared<CrouchingState>();
			}
			else if (player->falling)
			{
				return std::make_shared<FallingState>();
			}

			return nullptr;
		}

		/* Fall
		Controlled fall state, triggered from jump descent
		If falling for too long, becomes uncontrolled fall
		If landing, land with a clean crouch (allows for walking from landing)
		*/
		void FallingState::onEnter(PlayerBody *player)
		{
			time = 0.0f;
		}

		StatePtr FallingState::onUpdate(PlayerBody *player, float frameTime)
		{
			time += frameTime;
			if (time > 2.0f)
			{
				return std::make_shared<DroppingState>();
			}
			else if (player->touchingSurface)
			{
				return std::make_shared<CrouchingState>();
			}

			return nullptr;
		}

		/* Drop
		Uncontrolled fall state, triggered from lack of ground or jumping too far
		Lands in idle state (doesn't allow walking from fall)
		*/
		StatePtr DroppingState::onUpdate(PlayerBody *player, float frameTime)
		{
			if (player->touchingSurface)
			{
				return std::make_shared<IdleState>();
			}

			return nullptr;
		}

		Newton::EntityPtr createPlayerBody(Plugin::Population *population, NewtonWorld *newtonWorld, Plugin::Entity *entity)
		{
			std::shared_ptr<PlayerBody> player(std::make_shared<PlayerBody>(population, newtonWorld, entity));
            population->onAction.connect<PlayerBody, &PlayerBody::onAction>(player.get());
			return player;
		}
	}; // namespace Newton
}; // namespace Gek