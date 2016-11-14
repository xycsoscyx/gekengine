#include "GEK\Math\Common.hpp"
#include "GEK\Math\Matrix4x4.hpp"
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

        class ConvexCastFilter
        {
        protected:
            const NewtonBody *newtonBody;

        public:
            ConvexCastFilter(void)
                : newtonBody(nullptr)
            {
            }

            ConvexCastFilter(const NewtonBody* const newtonBody)
                :newtonBody(newtonBody)
            {
            }

            virtual unsigned PreFilter(const NewtonBody* const newtonBody, const NewtonCollision* const myCollision)
            {
                return NewtonCollisionGetMode(myCollision);
            }

            static unsigned PreFilter(const NewtonBody* const newtonBody, const NewtonCollision* const myCollision, void* const userData)
            {
                ConvexCastFilter* const filter = static_cast<ConvexCastFilter *>(userData);
                return (newtonBody != filter->newtonBody) ? filter->PreFilter(newtonBody, myCollision) : 0;
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

            bool touchingSurface = false;
            bool jumping = false;

        private:
            void setRestrainingDistance(float distance)
            {
                restrainingDistance = std::max(std::abs(distance), PLAYER_MIN_RESTRAINING_DISTANCE);
            }

            void setClimbSlope(float slopeInRadians)
            {
                maximumSlope = std::cos(std::abs(slopeInRadians));
            }

            Math::Float3 calculateDesiredOmega(const Math::Float4x4 &matrix, float frameTime) const
            {
                Math::Quaternion playerRotation(matrix.getRotation());
                Math::Quaternion targetRotation(Math::Quaternion::FromYaw(headingAngle));

                float scale = 1.0f;
                if (playerRotation.dot(targetRotation) < 0.0f)
                {
                    scale = -1.0f;
                }

                Math::Quaternion delta((playerRotation * scale).getInverse() * targetRotation);
                float deltaMagnitudeSquared = delta.getMagnitudeSquared();        
                if (deltaMagnitudeSquared	< (1.0e-5f * 1.0e-5f))
                {
                    return Math::Float3::Zero;
                }

                float inverseDeltaMagnitudeSquared = 1.0f / std::sqrt(deltaMagnitudeSquared);
                float normalizedLength = deltaMagnitudeSquared * inverseDeltaMagnitudeSquared;

                float inverseFrameTime = (0.5f / frameTime);
                float omegaLength = 2.0f * std::atan2(normalizedLength, delta.angle) * inverseFrameTime;
                return delta.axis * (inverseDeltaMagnitudeSquared * omegaLength);
            }

            Math::Float3 calculateDesiredVelocity(const Math::Float4x4 &matrix, const Math::Float3 &gravity, float frameTime) const
            {
                Math::Float3 playerXAxis(matrix.rx.xyz);
                Math::Float3 playerYAxis(matrix.ry.xyz);
                Math::Float3 playerZAxis(matrix.rz.xyz);

                Math::Float3 velocity;
                if (verticalSpeed <= 0.0f && groundNormal.getMagnitudeSquared() > 0.0f)
                {
                    // plane is supported by a ground plane, apply the player input velocity
                    if (groundNormal.dot(playerYAxis) >= maximumSlope)
                    {
                        // player is in a legal slope, he is in full control of his movement
                        Math::Float3 currentVelocity;
                        NewtonBodyGetVelocity(newtonBody, currentVelocity.data);
                        velocity = (playerYAxis * currentVelocity.dot(playerYAxis)) + (gravity * frameTime) + (playerZAxis * forwardSpeed) + (playerXAxis * lateralSpeed) + (playerYAxis * verticalSpeed);
                        velocity += (groundVelocity - (playerYAxis * playerYAxis.dot(groundVelocity)));

                        float speedMagnitudeSquared = velocity.getMagnitudeSquared();
                        float speedLimit = forwardSpeed * forwardSpeed + lateralSpeed * lateralSpeed + verticalSpeed * verticalSpeed + groundVelocity.getMagnitudeSquared() + 0.1f;
                        if (speedMagnitudeSquared > speedLimit)
                        {
                            velocity = (velocity * std::sqrt(speedLimit / speedMagnitudeSquared));
                        }

                        float friction = groundNormal.dot(velocity - groundVelocity);
                        if (friction < 0.0f)
                        {
                            velocity -= (groundNormal * friction);
                        }
                    }
                    else
                    {
                        // player is in an illegal ramp, he slides down hill an loses control of his movement 
                        NewtonBodyGetVelocity(newtonBody, velocity.data);
                        velocity += (playerYAxis * verticalSpeed);
                        velocity += (gravity * frameTime);
                        float friction = groundNormal.dot(velocity - groundVelocity);
                        if (friction < 0.0f)
                        {
                            velocity -= (groundNormal * friction);
                        }
                    }
                }
                else
                {
                    // player is on free fall, only apply the gravity
                    NewtonBodyGetVelocity(newtonBody, velocity.data);
                    velocity += (playerYAxis * verticalSpeed);
                    velocity += (gravity * frameTime);
                }

                return velocity;
            }

            void setPlayerVelocity(const Math::Float4x4 &matrix, const Math::Float3 &gravity, float frameTime)
            {
                Math::Float3 omega(calculateDesiredOmega(matrix, frameTime));
                NewtonBodySetOmega(newtonBody, omega.data);

                Math::Float3 velocity(calculateDesiredVelocity(matrix, gravity, frameTime));
                NewtonBodySetVelocity(newtonBody, velocity.data);
            }

            float calculateContactKinematics(const Math::Float3 &velocity, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
            {
                Math::Float3 contactVelocity(0.0f);
                if (contactInfo->m_hitBody)
                {
                    NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, contactVelocity.data);
                }

                const float restitution = 0.0f;
                Math::Float3 normal(contactInfo->m_normal);
                float reboundVelocMag = -(velocity - contactVelocity).dot(normal) * (1.0f + restitution);
                return (reboundVelocMag > 0.0f) ? reboundVelocMag : 0.0f;
            }

            void updateGroundPlane(Math::Float4x4 &matrix, const Math::Float4x4 &castMatrix, const Math::Float3 &destination, int threadHandle)
            {
                groundNormal.set(0.0f);
                groundVelocity.set(0.0f);

                float distance = 10.0f;
                ConvexCastFilter filter(newtonBody);
                NewtonWorldConvexCastReturnInfo castInformation;
                int count = NewtonWorldConvexCast(newtonWorld, castMatrix.data, destination.data, newtonCastingShape, &distance, &filter, ConvexCastFilter::PreFilter, &castInformation, 1, threadHandle);
                if (touchingSurface = (count && (distance <= 1.0f)))
                {
                    groundNormal.set(castInformation.m_normal);
                    Math::Float3 supportPoint(castMatrix.translation.xyz + ((destination - castMatrix.translation.xyz) * distance));
                    NewtonBodyGetPointVelocity(castInformation.m_hitBody, supportPoint.data, groundVelocity.data);
                    matrix.translation.xyz = supportPoint;
                    matrix.translation.w = 1.0f;
                }
            }

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

                setRestrainingDistance(0.01f);
                setClimbSlope(Math::DegreesToRadians(45.0f));

                const int stepCount = 12;
                Math::Float3 convexPoints[2][stepCount];

                // create an inner thin cylinder
                Math::Float3 point0(playerComponent.innerRadius, 0.0f, 0.0f);
                Math::Float3 point1(playerComponent.innerRadius, playerComponent.height, 0.0f);
                for (int step = 0; step < stepCount; step++)
                {
                    Math::Float4x4 rotation(Math::Float4x4::FromYaw(step * 2.0f * Math::Pi / stepCount));
                    convexPoints[0][step] = rotation.rotate(point0);
                    convexPoints[1][step] = rotation.rotate(point1);
                }

                NewtonCollision* const supportShape = NewtonCreateConvexHull(newtonWorld, stepCount * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, nullptr);

                // create the outer thick cylinder
                Math::Float4x4 outerShapeMatrix(Math::Float4x4::Identity);
                float capsuleHigh = playerComponent.height - playerComponent.stairStep;
                outerShapeMatrix.translation.xyz = (outerShapeMatrix.ry.xyz * (capsuleHigh * 0.5f + playerComponent.stairStep));

                NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.25f, 0.5f, 0, outerShapeMatrix.data);
                NewtonCollisionSetScale(bodyCapsule, playerComponent.outerRadius * 4.0f, capsuleHigh, playerComponent.outerRadius * 4.0f);

                // compound collision player controller
                NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
                NewtonCompoundCollisionBeginAddRemove(playerShape);
                NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
                NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
                NewtonCompoundCollisionEndAddRemove(playerShape);

                // create the kinematic body
                newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, transformComponent.getMatrix().data);
                NewtonBodySetUserData(newtonBody, static_cast<Newton::Entity *>(this));

                // players must have weight, otherwise they are infinitely strong when they collide
                NewtonCollision* const shape = NewtonBodyGetCollision(newtonBody);
                NewtonBodySetMassProperties(newtonBody, physicalComponent.mass, shape);

                // make the body collidable with other dynamics bodies, by default
                NewtonBodySetCollidable(newtonBody, true);

                float castHeight = capsuleHigh * 0.4f;
                float castRadius = (playerComponent.innerRadius * 0.5f > 0.05f) ? playerComponent.innerRadius * 0.5f : 0.05f;
                point0.set(castRadius, 0.0f, 0.0f);
                point1.set(castRadius, castHeight, 0.0f);
                for (int step = 0; step < stepCount; step++)
                {
                    Math::Float4x4 rotation(Math::Float4x4::FromYaw(step * 2.0f * Math::Pi / stepCount));
                    convexPoints[0][step] = rotation.rotate(point0);
                    convexPoints[1][step] = rotation.rotate(point1);
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

            void onPreUpdate(float frameTime, int threadHandle)
			{
                forwardSpeed = 0.0f;
                lateralSpeed = 0.0f;
                verticalSpeed = 0.0f;
                StatePtr nextState(currentState->onUpdate(this, frameTime));
				if (nextState)
				{
					currentState->onExit(this);
					nextState->onEnter(this);
					currentState = nextState;
				}

                Math::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);
                auto gravity(world->getGravity(matrix.translation.xyz));
                setPlayerVelocity(matrix, gravity, frameTime);
            }

            void onPostUpdate(float frameTime, int threadHandle)
			{
                const auto &playerComponent = entity->getComponent<Components::Player>();

                Math::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);

                Math::Float3 velocity;
                NewtonBodyGetVelocity(newtonBody, velocity.data);

                Math::Float3 omega;
                NewtonBodyGetOmega(newtonBody, omega.data);

                // integrate body angular velocity
                float omegaMagnitudeSquared = omega.getMagnitudeSquared();
                const float ErrorAngle = 0.0125f * 3.141592f / 180.0f;
                const float ErrorAngleSquared = ErrorAngle * ErrorAngle;
                if (omegaMagnitudeSquared > ErrorAngleSquared)
                {
                    float inverseomegaMagnitudeSquared = 1.0f / std::sqrt(omegaMagnitudeSquared);
                    Math::Float3 omegaAxis(omega * (inverseomegaMagnitudeSquared));
                    float omegaAngle = inverseomegaMagnitudeSquared * omegaMagnitudeSquared * frameTime;
                    Math::Quaternion deltaRotation(Math::Quaternion::FromAngular(omegaAxis, omegaAngle));
                    matrix.setRotation((matrix.getRotation() * deltaRotation).getNormal());
                }

                // integrate linear velocity
                float normalizedTimeLeft = 1.0f;
                float descreteTimeStep = frameTime * (1.0f / D_DESCRETE_MOTION_STEPS);
                int previousContactCount = 0;
                ConvexCastFilter castFilterData(newtonBody);
                NewtonWorldConvexCastReturnInfo previousContactList[PLAYER_CONTROLLER_MAX_CONTACTS];

                Math::Float3 playerYAxis(matrix.ry.xyz);

                Math::Float3 scale;
                NewtonCollisionGetScale(newtonUpperBodyShape, &scale.x, &scale.y, &scale.z);

                const float radius = (playerComponent.outerRadius + restrainingDistance) * 4.0f;
                NewtonCollisionSetScale(newtonUpperBodyShape, radius, playerComponent.height - playerComponent.stairStep, radius);

                Math::Float3 worldUpDirection(0.0f, 1.0f, 0.0f);
                NewtonWorldConvexCastReturnInfo worldUpConstraint;
                memset(&worldUpConstraint, 0, sizeof(worldUpConstraint));
                worldUpConstraint.m_normal[0] = worldUpDirection.x;
                worldUpConstraint.m_normal[1] = worldUpDirection.y;
                worldUpConstraint.m_normal[2] = worldUpDirection.z;
                worldUpConstraint.m_normal[3] = 0.0f;

                for (int j = 0; (j < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedTimeLeft > 1.0e-5f); j++)
                {
                    float velocityMagnitudeSquared = velocity.getMagnitudeSquared();
                    if (velocityMagnitudeSquared < 1.0e-6f)
                    {
                        break;
                    }

                    float timetoImpact;
                    NewtonWorldConvexCastReturnInfo currentCastList[PLAYER_CONTROLLER_MAX_CONTACTS];
                    Math::Float3 destinationPoint(matrix.translation.xyz + (velocity * frameTime));
                    int contactCount = NewtonWorldConvexCast(newtonWorld, matrix.data, destinationPoint.data, newtonUpperBodyShape, &timetoImpact, &castFilterData, ConvexCastFilter::PreFilter, currentCastList, PLAYER_CONTROLLER_MAX_CONTACTS, threadHandle);
                    if (contactCount)
                    {
                        //contactCount = manager->ProcessContacts(this, currentCastList, contactCount);
                    }

                    if (contactCount)
                    {
                        matrix.translation.xyz += velocity * (timetoImpact * frameTime);
                        if (timetoImpact > 0.0f)
                        {
                            matrix.translation.xyz -= velocity * (D_PLAYER_CONTACT_SKIN_THICKNESS / std::sqrt(velocityMagnitudeSquared));
                        }

                        normalizedTimeLeft -= timetoImpact;
                        float contactSpeedList[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                        float contactBoundList[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                        Math::Float3 contactNormalList[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                        for (int i = 1; i < contactCount; i++)
                        {
                            Math::Float3 n0(currentCastList[i - 1].m_normal);
                            for (int k = 0; k < i; k++)
                            {
                                Math::Float3 n1(currentCastList[k].m_normal);
                                if (n0.dot(n1) > 0.9999f)
                                {
                                    currentCastList[i] = currentCastList[contactCount - 1];
                                    i--;
                                    contactCount--;
                                    break;
                                }
                            }
                        }

                        int count = 0;
                        if (!jumping && touchingSurface)
                        {
                            worldUpConstraint.m_point[0] = matrix.translation.x;
                            worldUpConstraint.m_point[1] = matrix.translation.y;
                            worldUpConstraint.m_point[2] = matrix.translation.z;
                            worldUpConstraint.m_point[3] = matrix.translation.w;

                            contactSpeedList[count] = 0.0f;
                            contactNormalList[count].set(worldUpConstraint.m_normal);
                            contactBoundList[count] = calculateContactKinematics(velocity, &worldUpConstraint);
                            count++;
                        }

                        for (int i = 0; i < contactCount; i++)
                        {
                            contactSpeedList[count] = 0.0f;
                            contactNormalList[count].set(currentCastList[i].m_normal);
                            contactBoundList[count] = calculateContactKinematics(velocity, &currentCastList[i]);
                            count++;
                        }

                        for (int i = 0; i < previousContactCount; i++)
                        {
                            contactSpeedList[count] = 0.0f;
                            contactNormalList[count].set(previousContactList[i].m_normal);
                            contactBoundList[count] = calculateContactKinematics(velocity, &previousContactList[i]);
                            count++;
                        }

                        float residual = 10.0f;
                        Math::Float3 auxiliaryBounceVelocity(0.0f);
                        for (int i = 0; (i < D_PLAYER_MAX_SOLVER_ITERATIONS) && (residual > 1.0e-3f); i++)
                        {
                            residual = 0.0f;
                            for (int k = 0; k < count; k++)
                            {
                                Math::Float3 normal(contactNormalList[k]);
                                float v = contactBoundList[k] - normal.dot(auxiliaryBounceVelocity);
                                float x = contactSpeedList[k] + v;
                                if (x < 0.0f)
                                {
                                    v = 0.0f;
                                    x = 0.0f;
                                }

                                if (std::abs(v) > residual)
                                {
                                    residual = std::abs(v);
                                }

                                auxiliaryBounceVelocity += normal * (x - contactSpeedList[k]);
                                contactSpeedList[k] = x;
                            }
                        }

                        Math::Float3 velocityStep(0.0f);
                        for (int i = 0; i < count; i++)
                        {
                            Math::Float3 normal(contactNormalList[i]);
                            velocityStep += normal * (contactSpeedList[i]);
                        }

                        velocity += velocityStep;
                        if (velocityStep.getMagnitudeSquared() < 1.0e-6f)
                        {
                            float advanceTime = std::min(descreteTimeStep, normalizedTimeLeft * frameTime);
                            matrix.translation.xyz += velocity * (advanceTime);
                            normalizedTimeLeft -= advanceTime / frameTime;
                        }

                        previousContactCount = contactCount;
                        std::copy(std::begin(currentCastList), std::end(currentCastList), previousContactList);
                    }
                    else
                    {
                        matrix.translation.xyz = destinationPoint;
                        matrix.translation.w = 1.0f;
                        break;
                    }
                }

                NewtonCollisionSetScale(newtonUpperBodyShape, scale.x, scale.y, scale.z);

                // determine if player is standing on some plane
                Math::Float4x4 supportMatrix(matrix);
                supportMatrix.translation.xyz += (playerYAxis * sphereCastOrigin);
                if (jumping || !touchingSurface)
                {
                    Math::Float3 destination(matrix.translation);
                    updateGroundPlane(matrix, supportMatrix, destination, threadHandle);
                }
                else
                {
                    float step = std::abs(playerYAxis.dot(velocity * frameTime));
                    float castDistance = (groundNormal.dot(groundNormal) > 0.0f) ? playerComponent.stairStep : step;
                    Math::Float3 destination(matrix.translation - (playerYAxis * (castDistance * 2.0f)));
                    updateGroundPlane(matrix, supportMatrix, destination, threadHandle);
                }

                // set player velocity, position and orientation
                NewtonBodySetVelocity(newtonBody, velocity.data);
                NewtonBodySetMatrix(newtonBody, matrix.data);

                auto &transformComponent = entity->getComponent<Components::Transform>();
                transformComponent.setMatrix(matrix);
                transformComponent.position += matrix.ry.xyz * playerComponent.height;
            }
		};

		/* Idle
		If the ground drops out, the player starts to drop (uncontrolled)
		Actions trigger action states (crouch, walk, jump)
		*/
		StatePtr IdleState::onUpdate(PlayerBody *player, float frameTime)
		{
			return nullptr;
		}

		StatePtr IdleState::onAction(PlayerBody *player, const wchar_t *actionName, const Plugin::Population::ActionParameter &parameter)
		{
			if (_wcsicmp(actionName, L"crouch") == 0 && parameter.state)
			{
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