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
			const NewtonBody* sourceBody;

		public:
			ConvexCastPreFilter(const NewtonBody *sourceBody = nullptr)
				:sourceBody(sourceBody)
			{
			}

			static unsigned preFilter(const NewtonBody* const hitBody, const NewtonCollision* const, void* const userData)
			{
				ConvexCastPreFilter* const filterData = (ConvexCastPreFilter*)userData;
				if (hitBody == filterData->sourceBody)
				{
					return 0;
				}
				else
				{
					const NewtonCollision* const collision = NewtonBodyGetCollision(hitBody);
					return NewtonCollisionGetMode(collision);
				}
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
            float halfHeight = 0.0f;
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

                setRestrainingDistance(0.0f);
                setClimbSlope(45.0f * 3.1416f / 180.0f);

                const int stepCount = 12;
                Math::Float3 convexPoints[2][stepCount];

                // create an inner thin cylinder
                Math::Float3 p0(0.0f, playerComponent.innerRadius, 0.0f);
                Math::Float3 p1(playerComponent.height, playerComponent.innerRadius, 0.0f);
                for (int step = 0; step < stepCount; step++)
                {
                    Math::SIMD::Float4x4 rotation(Math::SIMD::Float4x4::createPitchRotation(step * 2.0f * 3.141592f / stepCount));
                    convexPoints[0][step] = rotation.rotate(p0);
                    convexPoints[1][step] = rotation.rotate(p1);
                }

                NewtonCollision* const supportShape = NewtonCreateConvexHull(newtonWorld, stepCount * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, NULL);

                // create the outer thick cylinder
                Math::SIMD::Float4x4 outerShapeMatrix(Math::SIMD::Float4x4::Identity);
                float capsuleHigh = playerComponent.height - playerComponent.stairStep;
                outerShapeMatrix.translation = outerShapeMatrix.ny * (capsuleHigh * 0.5f + playerComponent.stairStep);
                outerShapeMatrix.tw = 1.0f;

                NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.25f, 0.5f, 0, outerShapeMatrix.data);
                NewtonCollisionSetScale(bodyCapsule, capsuleHigh, playerComponent.outerRadius * 4.0f, playerComponent.outerRadius * 4.0f);

                // compound collision player controller
                NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
                NewtonCompoundCollisionBeginAddRemove(playerShape);
                NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
                NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
                NewtonCompoundCollisionEndAddRemove(playerShape);

                // create the kinematic body
                Math::SIMD::Float4x4 locationMatrix(Math::SIMD::Float4x4::Identity);
                newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, locationMatrix.data);

                // players must have weight, otherwise they are infinitely strong when they collide
                NewtonCollision* const shape = NewtonBodyGetCollision(newtonBody);
                NewtonBodySetMassProperties(newtonBody, physicalComponent.mass, shape);

                // make the body collidable with other dynamics bodies, by default
                NewtonBodySetCollidable(newtonBody, true);

                float castHeight = capsuleHigh * 0.4f;
                float castRadius = (playerComponent.innerRadius * 0.5f > 0.05f) ? playerComponent.innerRadius * 0.5f : 0.05f;
                Math::Float3 q0(0.0f, castRadius, 0.0f);
                Math::Float3 q1(castHeight, castRadius, 0.0f);
                for (int step = 0; step < stepCount; step++)
                {
                    Math::SIMD::Float4x4 rotation(Math::SIMD::Float4x4::createPitchRotation(step * 2.0f * 3.141592f / stepCount));
                    convexPoints[0][step] = rotation.rotate(q0);
                    convexPoints[1][step] = rotation.rotate(q1);
                }

                newtonCastingShape = NewtonCreateConvexHull(newtonWorld, stepCount * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, NULL);
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
                Math::FloatQuat playerRotation;
                NewtonBodyGetRotation(newtonBody, playerRotation.data);

                Math::FloatQuat targetRotation(Math::FloatQuat::createAngularRotation(Math::Float3(0.0f, 1.0f, 0.0f), headingAngle));
                return playerRotation.calculateAverageOmega(targetRotation, 0.5f / frameTime);
            }
            
            Math::Float3 calculateDesiredVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed, const Math::Float3& gravity, float frameTime) const
            {
                Math::SIMD::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);
                Math::Float3 updir(matrix.ny);
                Math::Float3 frontDir(matrix.nz);
                Math::Float3 rightDir(matrix.nx);

                Math::Float3 veloc;
                if ((verticalSpeed <= 0.0f) && (groundNormal.dot(groundNormal)) > 0.0f)
                {
                    // plane is supported by a ground plane, apply the player input velocity
                    if (groundNormal.dot(updir) >= maximumSlope)
                    {
                        // player is in a legal slope, he is in full control of his movement
                        Math::Float3 bodyVeloc;
                        NewtonBodyGetVelocity(newtonBody, bodyVeloc.data);
                        veloc = updir * bodyVeloc.dot(updir) + (gravity * frameTime) + (frontDir * forwardSpeed) + (rightDir * lateralSpeed) +( updir * verticalSpeed);
                        veloc += (groundVelocity - updir * updir.dot(groundVelocity));

                        float speedLimitMag2 = forwardSpeed * forwardSpeed + lateralSpeed * lateralSpeed + verticalSpeed * verticalSpeed + groundVelocity.dot(groundVelocity) + 0.1f;
                        float speedMag2 = veloc.dot(veloc);
                        if (speedMag2 > speedLimitMag2)
                        {
                            veloc = veloc * std::sqrt(speedLimitMag2 / speedMag2);
                        }

                        float normalVeloc = groundNormal.dot(veloc - groundVelocity);
                        if (normalVeloc < 0.0f)
                        {
                            veloc -= groundNormal * normalVeloc;
                        }
                    }
                    else
                    {
                        // player is in an illegal ramp, he slides down hill an loses control of his movement 
                        NewtonBodyGetVelocity(newtonBody, veloc.data);
                        veloc += updir * verticalSpeed;
                        veloc += gravity * frameTime;
                        float normalVeloc = groundNormal.dot(veloc - groundVelocity);
                        if (normalVeloc < 0.0f)
                        {
                            veloc -= groundNormal * normalVeloc;
                        }
                    }
                }
                else
                {
                    // player is on free fall, only apply the gravity
                    NewtonBodyGetVelocity(newtonBody, veloc.data);
                    veloc += updir * verticalSpeed;
                    veloc += gravity * frameTime;
                }

                return veloc;
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

                Math::Float3 veloc(calculateDesiredVelocity(forwardSpeed, lateralSpeed, verticalSpeed, gravity, frameTime));
                NewtonBodySetVelocity(newtonBody, veloc.data);
			}

            void updateGroundPlane(Math::SIMD::Float4x4& matrix, const Math::SIMD::Float4x4& castMatrix, const Math::Float3& distance, int threadHandle)
            {
                NewtonWorldConvexCastReturnInfo info;
                ConvexCastPreFilter filter(newtonBody);

                float param = 10.0f;
                int count = NewtonWorldConvexCast(newtonWorld, &castMatrix[0][0], distance.data, newtonCastingShape, &param, &filter, ConvexCastPreFilter::preFilter, &info, 1, threadHandle);

                groundNormal = Math::Float3::Zero;
                groundVelocity = Math::Float3::Zero;
                if (count && (param <= 1.0f))
                {
                    touchingSurface = true;
                    Math::Float3 supportPoint(castMatrix.translation + (distance - castMatrix.translation) * param);
                    groundNormal = Math::Float3(info.m_normal);
                    NewtonBodyGetPointVelocity(info.m_hitBody, supportPoint.data, groundVelocity.data);
                    matrix.translation = supportPoint;
                    matrix.tw = 1.0f;
                }
                else
                {
                    touchingSurface = false;
                }
            }
            
            float calculateContactKinematics(const Math::Float3& veloc, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
            {
                Math::Float3 contactVeloc;
                if (contactInfo->m_hitBody)
                {
                    NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, contactVeloc.data);
                }
                else
                {
                    contactVeloc = Math::Float3::Zero;
                }

                const float restitution = 0.0f;
                Math::Float3 normal(contactInfo->m_normal);
                float reboundVelocMag = -(veloc - contactVeloc).dot(normal) * (1.0f + restitution);
                return (reboundVelocMag > 0.0f) ? reboundVelocMag : 0.0f;
            }

            void onPostUpdate(float frameTime, int threadHandle)
			{
                const auto &playerComponent = entity->getComponent<Components::Player>();

                Math::SIMD::Float4x4 matrix;
                Math::FloatQuat bodyRotation;
                Math::Float3 veloc(Math::Float3::Zero);
                Math::Float3 omega(Math::Float3::Zero);

                // get the body motion state 
                NewtonBodyGetMatrix(newtonBody, &matrix[0][0]);
                NewtonBodyGetVelocity(newtonBody, &veloc[0]);
                NewtonBodyGetOmega(newtonBody, &omega[0]);

                // integrate body angular velocity
                NewtonBodyGetRotation(newtonBody, bodyRotation.data);
                bodyRotation = bodyRotation.integrateOmega(omega, frameTime);
                matrix = Math::convert(bodyRotation, matrix.translation);

                // integrate linear velocity
                float normalizedTimeLeft = 1.0f;
                float step = frameTime * std::sqrt(veloc.dot(veloc));
                float descreteTimeStep = frameTime * (1.0f / D_DESCRETE_MOTION_STEPS);
                int prevContactCount = 0;
                ConvexCastPreFilter castFilterData(newtonBody);
                NewtonWorldConvexCastReturnInfo prevInfo[PLAYER_CONTROLLER_MAX_CONTACTS];

                Math::Float3 updir(matrix.ny);

                Math::Float3 scale;
                NewtonCollisionGetScale(newtonUpperBodyShape, &scale.x, &scale.y, &scale.z);
                const float radio = (playerComponent.outerRadius + restrainingDistance) * 4.0f;
                NewtonCollisionSetScale(newtonUpperBodyShape, playerComponent.height - playerComponent.stairStep, radio, radio);

                NewtonWorldConvexCastReturnInfo upConstratint;
                memset(&upConstratint, 0, sizeof(upConstratint));
                upConstratint.m_normal[0] = 0.0f;
                upConstratint.m_normal[1] = 1.0f;
                upConstratint.m_normal[2] = 0.0f;
                upConstratint.m_normal[3] = 0.0f;

                for (int j = 0; (j < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedTimeLeft > 1.0e-5f); j++)
                {
                    if (veloc.dot(veloc) < 1.0e-6f)
                    {
                        break;
                    }

                    float timetoImpact;
                    NewtonWorldConvexCastReturnInfo info[PLAYER_CONTROLLER_MAX_CONTACTS];
                    Math::Float3 destPosit(matrix.translation + (veloc * frameTime));
                    int contactCount = NewtonWorldConvexCast(newtonWorld, &matrix[0][0], &destPosit[0], newtonUpperBodyShape, &timetoImpact, &castFilterData, ConvexCastPreFilter::preFilter , info, sizeof(info) / sizeof(info[0]), threadHandle);
                    if (contactCount)
                    {
                        //contactCount = manager->ProcessContacts(this, info, contactCount);
                    }

                    if (contactCount)
                    {
                        matrix.translation += veloc * (timetoImpact * frameTime);
                        if (timetoImpact > 0.0f)
                        {
                            matrix.translation -= veloc * (D_PLAYER_CONTACT_SKIN_THICKNESS / std::sqrt(veloc.dot(veloc)));
                        }

                        normalizedTimeLeft -= timetoImpact;

                        float speed[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                        float bounceSpeed[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                        Math::Float3 bounceNormal[PLAYER_CONTROLLER_MAX_CONTACTS * 2];

                        for (int i = 1; i < contactCount; i++)
                        {
                            Math::Float3 n0(info[i - 1].m_normal);
                            for (int k = 0; k < i; k++)
                            {
                                Math::Float3 n1(info[k].m_normal);
                                if (n0.dot(n1) > 0.9999f)
                                {
                                    info[i] = info[contactCount - 1];
                                    i--;
                                    contactCount--;
                                    break;
                                }
                            }
                        }

                        int count = 0;
                        if (touchingSurface)
                        {
                            upConstratint.m_point[0] = matrix.translation.x;
                            upConstratint.m_point[1] = matrix.translation.y;
                            upConstratint.m_point[2] = matrix.translation.z;
                            upConstratint.m_point[3] = 0.0f;

                            speed[count] = 0.0f;
                            bounceNormal[count] = Math::Float3(upConstratint.m_normal);
                            bounceSpeed[count] = calculateContactKinematics(veloc, &upConstratint);
                            count++;
                        }

                        for (int i = 0; i < contactCount; i++)
                        {
                            speed[count] = 0.0f;
                            bounceNormal[count] = Math::Float3(info[i].m_normal);
                            bounceSpeed[count] = calculateContactKinematics(veloc, &info[i]);
                            count++;
                        }

                        for (int i = 0; i < prevContactCount; i++)
                        {
                            speed[count] = 0.0f;
                            bounceNormal[count] = Math::Float3(prevInfo[i].m_normal);
                            bounceSpeed[count] = calculateContactKinematics(veloc, &prevInfo[i]);
                            count++;
                        }

                        float residual = 10.0f;
                        Math::Float3 auxBounceVeloc(0.0f, 0.0f, 0.0f);
                        for (int i = 0; (i < D_PLAYER_MAX_SOLVER_ITERATIONS) && (residual > 1.0e-3f); i++)
                        {
                            residual = 0.0f;
                            for (int k = 0; k < count; k++)
                            {
                                Math::Float3 normal(bounceNormal[k]);
                                float v = bounceSpeed[k] - normal.dot(auxBounceVeloc);
                                float x = speed[k] + v;
                                if (x < 0.0f)
                                {
                                    v = 0.0f;
                                    x = 0.0f;
                                }

                                if (std::abs(v) > residual)
                                {
                                    residual = std::abs(v);
                                }

                                auxBounceVeloc += normal * (x - speed[k]);
                                speed[k] = x;
                            }
                        }

                        Math::Float3 velocStep(0.0f);
                        for (int i = 0; i < count; i++)
                        {
                            Math::Float3 normal(bounceNormal[i]);
                            velocStep += normal * speed[i];
                        }

                        veloc += velocStep;
                        float velocMag2 = velocStep.dot(velocStep);
                        if (velocMag2 < 1.0e-6f)
                        {
                            float advanceTime = std::min(descreteTimeStep, normalizedTimeLeft * frameTime);
                            matrix.translation += veloc * advanceTime;
                            normalizedTimeLeft -= advanceTime / frameTime;
                        }

                        prevContactCount = contactCount;
                        memcpy(prevInfo, info, prevContactCount * sizeof(NewtonWorldConvexCastReturnInfo));

                    }
                    else
                    {
                        matrix.translation = destPosit;
                        matrix.tw = 1.0f;
                        break;
                    }
                }

                NewtonCollisionSetScale(newtonUpperBodyShape, scale.x, scale.y, scale.z);

                // determine if player is standing on some plane
                Math::SIMD::Float4x4 supportMatrix(matrix);
                supportMatrix.translation += updir * sphereCastOrigin;
                if (touchingSurface)
                {
                    step = std::abs(updir.dot(veloc * frameTime));
                    float castDist = (groundNormal.dot(groundNormal) > 0.0f) ? playerComponent.stairStep : step;
                    Math::Float3 distance(matrix.translation - updir * (castDist * 2.0f));
                    updateGroundPlane(matrix, supportMatrix, distance, threadHandle);
                }
                else
                {
                    Math::Float3 distance(matrix.translation);
                    updateGroundPlane(matrix, supportMatrix, distance, threadHandle);
                }

                // set player velocity, position and orientation
                NewtonBodySetVelocity(newtonBody, veloc.data);
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