#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Newton/Base.hpp"
#include <algorithm>
#include <memory>

#include <Newton.h>

namespace Gek
{
	namespace Newton
	{
        static const float NewtonEpsilon = 1.0e-5f;
        static const float MinimumRestrainingDistance = 1.0e-2f;
        static const float ContactSkinThickness = 0.025f;
        static const uint32_t MaximumContactCount = 32;
        static const uint32_t DiscreteMotionStepCount = 8;
        static const uint32_t MaximumIntegrationStepCount = 8;
        static const uint32_t MaximumSolverStepCount = 16;

        class ConvexCastFilter
        {
        protected:
            const NewtonBody *sourceBody = nullptr;

        public:
            ConvexCastFilter(const NewtonBody* const sourceBody)
                : sourceBody(sourceBody)
            {
            }

            virtual unsigned PreFilter(const NewtonBody* const collisionBody, const NewtonCollision* const collisionObject)
            {
                return NewtonCollisionGetMode(collisionObject);
            }

            static unsigned PreFilter(const NewtonBody* const collisionBody, const NewtonCollision* const collisionObject, void* const userData)
            {
                ConvexCastFilter* const filter = static_cast<ConvexCastFilter *>(userData);
                return (collisionBody != filter->sourceBody) ? filter->PreFilter(collisionBody, collisionObject) : 0;
            }
        };

		class PlayerBody;

		GEK_INTERFACE(State)
		{
            virtual ~State(void) = default;

            virtual void onEnter(PlayerBody *player) { };
			virtual void onExit(PlayerBody *player) { };

			virtual StatePtr onUpdate(PlayerBody *player, float frameTime) { return nullptr; };
			virtual StatePtr onAction(PlayerBody *player, Plugin::Population::Action const &action) { return nullptr; };
		};

		class IdleState
			: public State
		{
		public:
			StatePtr onUpdate(PlayerBody *player, float frameTime);
			StatePtr onAction(PlayerBody *player, Plugin::Population::Action const &action);
		};

		class WalkingState
			: public State
		{
		public:
			StatePtr onUpdate(PlayerBody *player, float frameTime);
			StatePtr onAction(PlayerBody *player, Plugin::Population::Action const &action);
		};

		class JumpingState
			: public State
		{
		public:
			void onEnter(PlayerBody *player);
			StatePtr onUpdate(PlayerBody *player, float frameTime);
            StatePtr onAction(PlayerBody *player, Plugin::Population::Action const &action);
        };

		class PlayerBody
			: public Newton::Entity
		{
		public:
            Plugin::Core *core = nullptr;
            Plugin::Population *population = nullptr;
			Newton::World *world = nullptr;
			NewtonWorld *newtonWorld = nullptr;

            Plugin::Entity * const entity = nullptr;
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
            float lookingAngle = 0.0f;
			float forwardSpeed = 0.0f;
			float lateralSpeed = 0.0f;
			float verticalSpeed = 0.0f;

            Math::Float3 groundNormal = Math::Float3::Zero;
            Math::Float3 groundVelocity = Math::Float3::Zero;

            bool touchingSurface = false;

        public:
			PlayerBody(Plugin::Core *core,
                Plugin::Population *population,
				NewtonWorld *newtonWorld,
				Plugin::Entity * const entity)
                : core(core)
				, population(population)
				, world(static_cast<Newton::World *>(NewtonWorldGetUserData(newtonWorld)))
				, newtonWorld(newtonWorld)
				, entity(entity)
				, currentState(std::make_unique<IdleState>())
			{
                auto const &physicalComponent = entity->getComponent<Components::Physical>();
				auto const &transformComponent = entity->getComponent<Components::Transform>();
				auto const &playerComponent = entity->getComponent<Components::Player>();

                setRestrainingDistance(0.0f);
                setClimbSlope(Math::DegreesToRadians(45.0f));

                headingAngle = transformComponent.rotation.getEuler().y;

                const int convexShapeDensity = 12;
                Math::Float3 convexPoints[2][convexShapeDensity];

                // create an inner thin cylinder
                Math::Float3 point0(playerComponent.innerRadius, 0.0f, 0.0f);
                Math::Float3 point1(playerComponent.innerRadius, playerComponent.height, 0.0f);
                for (int point = 0; point < convexShapeDensity; ++point)
                {
                    Math::Float4x4 rotation(Math::Float4x4::MakeYawRotation(point * 2.0f * Math::Pi / convexShapeDensity));
                    convexPoints[0][point] = rotation.rotate(point0);
                    convexPoints[1][point] = rotation.rotate(point1);
                }

                NewtonCollision* const supportShape = NewtonCreateConvexHull(newtonWorld, convexShapeDensity * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, nullptr);

                // create the outer thick cylinder
                Math::Float4x4 outerShapeMatrix(Math::Float4x4::Identity);
                float capsuleHigh = playerComponent.height - playerComponent.stairStep;
                sphereCastOrigin = capsuleHigh * 0.5f + playerComponent.stairStep;
                outerShapeMatrix.translation.xyz = (outerShapeMatrix.ry.xyz * sphereCastOrigin);
                NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.25f, 0.5f, 0, outerShapeMatrix.data);
                NewtonCollisionSetScale(bodyCapsule, playerComponent.outerRadius * 4.0f, capsuleHigh, playerComponent.outerRadius * 4.0f);

                // compound collision player controller
                NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
                NewtonCompoundCollisionBeginAddRemove(playerShape);
                NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
                NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
                NewtonCompoundCollisionEndAddRemove(playerShape);

                // create the kinematic body
                auto matrix(transformComponent.getMatrix());
                matrix.translation.xyz -= (matrix.ry.xyz * playerComponent.height);
                newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, matrix.data);
                NewtonBodySetUserData(newtonBody, static_cast<Newton::Entity *>(this));

                // players must have weight, otherwise they are infinitely strong when they collide
                NewtonCollision* const shape = NewtonBodyGetCollision(newtonBody);
                NewtonBodySetMassProperties(newtonBody, physicalComponent.mass, shape);

                // make the body collidable with other dynamics bodies, by default
                NewtonBodySetCollidable(newtonBody, true);
                NewtonBodySetAutoSleep(newtonBody, false);

                float castHeight = capsuleHigh * 0.4f;
                float castRadius = (playerComponent.innerRadius * 0.5f > 0.05f) ? playerComponent.innerRadius * 0.5f : 0.05f;

                point0.set(castRadius, 0.0f, 0.0f);
                point1.set(castRadius, castHeight, 0.0f);
                for (int point = 0; point < convexShapeDensity; ++point)
                {
                    Math::Float4x4 rotation(Math::Float4x4::MakeYawRotation(point * 2.0f * Math::Pi / convexShapeDensity));
                    convexPoints[0][point] = rotation.rotate(point0);
                    convexPoints[1][point] = rotation.rotate(point1);
                }
                
                newtonCastingShape = NewtonCreateConvexHull(newtonWorld, convexShapeDensity * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, nullptr);
                newtonSupportShape = NewtonCompoundCollisionGetCollisionFromNode(shape, NewtonCompoundCollisionGetNodeByIndex(shape, 0));
                newtonUpperBodyShape = NewtonCompoundCollisionGetCollisionFromNode(shape, NewtonCompoundCollisionGetNodeByIndex(shape, 1));

                NewtonDestroyCollision(bodyCapsule);
                NewtonDestroyCollision(supportShape);
                NewtonDestroyCollision(playerShape);

                population->onAction.connect(this, &PlayerBody::onAction);
            }

			~PlayerBody(void)
			{
                NewtonDestroyCollision(newtonCastingShape);
			}

            void setRestrainingDistance(float distance)
            {
                restrainingDistance = std::max(std::abs(distance), float(MinimumRestrainingDistance));
            }

            void setClimbSlope(float slopeInRadians)
            {
                maximumSlope = std::cos(std::abs(slopeInRadians));
            }

            Math::Float3 calculateAverageOmega(Math::Quaternion const &currentRotation, Math::Quaternion const &targetRotation, float inverseFrameTime) const
            {
                float scale = 1.0f;
                if (currentRotation.dot(targetRotation) < 0.0f)
                {
                    scale = -1.0f;
                }

                Math::Quaternion deltaRotation((currentRotation * scale).getInverse() * targetRotation);
                Math::Float3 omegaDirection(deltaRotation.axis);

                float directionMagnitude = omegaDirection.getMagnitude();
                if (directionMagnitude < (NewtonEpsilon * NewtonEpsilon))
                {
                    return Math::Float3::Zero;
                }

                float inversedirectionLength = 1.0f / std::sqrt(directionMagnitude);
                float directionLength = directionMagnitude * inversedirectionLength;

                float omegaMagnitude = 2.0f * std::atan2(directionLength, deltaRotation.angle) * inverseFrameTime;
                auto averageOmega(omegaDirection * (inversedirectionLength * omegaMagnitude));

                return averageOmega;
            }

            // New way, but doesn't work despite the comment from Newton code
            void integrateOmegaBAD(Math::Float4x4 &matrix, Math::Float3 const &omega, float frameTIme) const
            {
                // this is correct
                float omegaMagnitudeSquared = omega.getMagnitude();
                const float ErrorAngle = Math::DegreesToRadians(0.0125f);
                const float ErrorAngleSquared = ErrorAngle * ErrorAngle;
                if (omegaMagnitudeSquared > ErrorAngleSquared)
                {
                    float inverseOmegaMagnitude = 1.0f / std::sqrt(omegaMagnitudeSquared);
                    Math::Float3 omegaAxis(omega * inverseOmegaMagnitude);
                    float omegaAngle = inverseOmegaMagnitude * omegaMagnitudeSquared * frameTIme;
                    Math::Quaternion deltaRotation(omegaAxis, omegaAngle);
                    matrix.setRotation((matrix.getRotation() * deltaRotation).getNormal());
                }
            }

            // Old way, works but may not actually be correct?
            void integrateOmega(Math::Float4x4 &matrix, Math::Float3 const &omega, float frameTime) const
            {
                Math::Float3 theta(omega * frameTime * 0.5f);
                float thetaMagnitideSquared = theta.getMagnitude();

                float thetaMagnitude;
                Math::Quaternion deltaRotation;
                if ((thetaMagnitideSquared * thetaMagnitideSquared / 24.0f) < NewtonEpsilon)
                {
                    deltaRotation.w = (1.0f - (thetaMagnitideSquared / 2.0f));
                    thetaMagnitude = (1.0f - (thetaMagnitideSquared / 6.0f));
                }
                else
                {
                    thetaMagnitude = std::sqrt(thetaMagnitideSquared);
                    deltaRotation.w = std::cos(thetaMagnitude);
                    thetaMagnitude = (std::sin(thetaMagnitude) / thetaMagnitude);
                }

                deltaRotation.axis = (theta * thetaMagnitude);
                matrix.setRotation((matrix.getRotation() * deltaRotation).getNormal());
            }

            Math::Float3 calculateDesiredOmega(Math::Float4x4 const &matrix, float frameTime) const
            {
                Math::Quaternion playerRotation(matrix.getRotation());
                Math::Quaternion targetRotation(Math::Quaternion::MakeYawRotation(headingAngle));
                return calculateAverageOmega(playerRotation, targetRotation, 0.5f / frameTime);
            }

            Math::Float3 calculateDesiredVelocity(Math::Float4x4 const &matrix, Math::Float3 const &gravity, float frameTime) const
            {
                Math::Float3 const &playerXAxis = matrix.rx.xyz;
                Math::Float3 const &playerYAxis = matrix.ry.xyz;
                Math::Float3 const &playerZAxis = matrix.rz.xyz;

                Math::Float3 desiredVelocity;
                if (verticalSpeed <= 0.0f && groundNormal.dot(groundNormal) > 0.0f)
                {
                    // plane is supported by a ground plane, apply the player input desiredVelocity
                    if (groundNormal.dot(playerYAxis) >= maximumSlope)
                    {
                        // player is in a legal slope, he is in full control of his movement
                        Math::Float3 currentVelocity;
                        NewtonBodyGetVelocity(newtonBody, currentVelocity.data);
                        desiredVelocity = (playerYAxis * currentVelocity.dot(playerYAxis)) + (gravity * frameTime) + (playerZAxis * forwardSpeed) + (playerXAxis * lateralSpeed) + (playerYAxis * verticalSpeed);
                        desiredVelocity += (groundVelocity - (playerYAxis * playerYAxis.dot(groundVelocity)));

                        float speedLimitMagnitude = forwardSpeed * forwardSpeed + lateralSpeed * lateralSpeed + verticalSpeed * verticalSpeed + groundVelocity.getMagnitude() + 0.1f;
                        float speedMagnitude = desiredVelocity.getMagnitude();
                        if (speedMagnitude > speedLimitMagnitude)
                        {
                            desiredVelocity = desiredVelocity * std::sqrt(speedLimitMagnitude / speedMagnitude);
                        }

                        float friction = groundNormal.dot(desiredVelocity - groundVelocity);
                        if (friction < 0.0f)
                        {
                            desiredVelocity -= (groundNormal * friction);
                        }
                    }
                    else
                    {
                        // player is in an illegal ramp, he slides down hill an loses control of his movement 
                        NewtonBodyGetVelocity(newtonBody, desiredVelocity.data);
                        desiredVelocity += (playerYAxis * verticalSpeed);
                        desiredVelocity += (gravity * frameTime);
                        float friction = groundNormal.dot(desiredVelocity - groundVelocity);
                        if (friction < 0.0f)
                        {
                            desiredVelocity -= (groundNormal * friction);
                        }
                    }
                }
                else
                {
                    // player is on free fall, only apply the gravity
                    NewtonBodyGetVelocity(newtonBody, desiredVelocity.data);
                    desiredVelocity += (playerYAxis * verticalSpeed);
                    desiredVelocity += (gravity * frameTime);
                }

                return desiredVelocity;
            }

            void setPlayerVelocity(Math::Float4x4 const &matrix, Math::Float3 const &gravity, float frameTime)
            {
                Math::Float3 omega(calculateDesiredOmega(matrix, frameTime));
                Math::Float3 velocity(calculateDesiredVelocity(matrix, gravity, frameTime));

                NewtonBodySetOmega(newtonBody, omega.data);
                NewtonBodySetVelocity(newtonBody, velocity.data);
            }

            float calculateContactKinematics(Math::Float3 const &velocity, NewtonWorldConvexCastReturnInfo const * const contactInfo) const
            {
                Math::Float3 contactVelocity = Math::Float3::Zero;
                if (contactInfo->m_hitBody)
                {
                    NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, contactVelocity.data);
                }

                const float restitution = 0.0f;
                Math::Float3 normal(contactInfo->m_normal);
                float reboundVelocity = -(velocity - contactVelocity).dot(normal) * (1.0f + restitution);
                return (reboundVelocity > 0.0f) ? reboundVelocity : 0.0f;
            }

            void updateGroundPlane(Math::Float4x4& matrix, Math::Float4x4 const &castMatrix, Math::Float3 const &targetPoint, int threadHandle)
            {
                groundNormal = Math::Float3::Zero;
                groundVelocity = Math::Float3::Zero;

                float contactDistance = 10.0f;
                ConvexCastFilter filter(newtonBody);
                NewtonWorldConvexCastReturnInfo contactInformation;
                int count = NewtonWorldConvexCast(newtonWorld, castMatrix.data, targetPoint.data, newtonCastingShape, &contactDistance, &filter, ConvexCastFilter::PreFilter, &contactInformation, 1, threadHandle);
                if (count && (contactDistance <= 1.0f))
                {
                    touchingSurface = true;
                    auto supportPoint(castMatrix.translation.xyz + (targetPoint - castMatrix.translation.xyz) * contactDistance);
                    groundNormal.set(contactInformation.m_normal);
                    NewtonBodyGetPointVelocity(contactInformation.m_hitBody, supportPoint.data, groundVelocity.data);
                    matrix.translation.set(supportPoint, 1.0f);
                }
                else
                {
                    touchingSurface = false;
                }
            }

            // Plugin::Population Slots
			void onAction(Plugin::Population::Action const &action)
			{
                bool editorActive = core->getOption("editor", "active").convert(false);
                if (editorActive)
                {
                    return;
                }

                if (action.name == "turn")
				{
					headingAngle += (action.value * 0.01f);
				}
                else if (action.name == "tilt")
                {
                    lookingAngle += (action.value * 0.01f);
                    lookingAngle = Math::Clamp(lookingAngle, -Math::Pi * 0.5f, Math::Pi * 0.5f);
                }
                else if (action.name == "move_forward")
				{
					moveForward = action.state;
				}
				else if (action.name == "move_backward")
				{
					moveBackward = action.state;
				}
				else if (action.name == "strafe_left")
				{
					strafeLeft = action.state;
				}
				else if (action.name == "strafe_right")
				{
					strafeRight = action.state;
				}
				else if (action.name == "crouch")
				{
				}

				StatePtr nextState(currentState->onAction(this, action));
				if (nextState)
				{
					currentState->onExit(this);
					nextState->onEnter(this);
                    currentState = std::move(nextState);
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

			uint32_t getSurface(Math::Float3 const &position, Math::Float3 const &normal)
			{
				return 0;
			}

            void onPreUpdate(float frameTime, int threadHandle)
			{
                StatePtr nextState(currentState->onUpdate(this, frameTime));
				if (nextState)
				{
					currentState->onExit(this);
					nextState->onEnter(this);
                    currentState = std::move(nextState);
				}

                Math::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);
                auto gravity(world->getGravity(matrix.translation.xyz));
                setPlayerVelocity(matrix, gravity, frameTime);
            }

            void onPostUpdate(float frameTime, int threadHandle)
			{
                auto const &playerComponent = entity->getComponent<Components::Player>();

                // get the body motion state 
                Math::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);

                // integrate body angular velocity
                Math::Float3 omega;
                NewtonBodyGetOmega(newtonBody, omega.data);
                integrateOmega(matrix, omega, frameTime);

                // integrate linear velocity
                Math::Float3 velocity;
                NewtonBodyGetVelocity(newtonBody, velocity.data);

                float normalizedTimeLeft = 1.0f;
                float descreteTimeStep = frameTime * (1.0f / DiscreteMotionStepCount);
                int prevContactCount = 0;
                ConvexCastFilter castFilterData(newtonBody);
                std::array<NewtonWorldConvexCastReturnInfo, MaximumContactCount> previousContactList;

                Math::Float3 const &playerYAxis = matrix.ry.xyz;

                Math::Float3 scale(0.0f);
                NewtonCollisionGetScale(newtonUpperBodyShape, &scale.x, &scale.y, &scale.z);
                const float radius = (playerComponent.outerRadius + restrainingDistance) * 4.0f;
                NewtonCollisionSetScale(newtonUpperBodyShape, radius, playerComponent.height - playerComponent.stairStep, radius);

                NewtonWorldConvexCastReturnInfo upConstraint;
                memset(&upConstraint, 0, sizeof(upConstraint));
                upConstraint.m_normal[0] = 0.0f;
                upConstraint.m_normal[1] = 1.0f;
                upConstraint.m_normal[2] = 0.0f;
                upConstraint.m_normal[3] = 0.0f;

                for (uint32_t step = 0; (step < MaximumIntegrationStepCount) && (normalizedTimeLeft > NewtonEpsilon); step++)
                {
                    if (velocity.getMagnitude() < NewtonEpsilon)
                    {
                        break;
                    }

                    float impactTime;
                    Math::Float3 castTargetPosition(matrix.translation.xyz + (velocity * frameTime));
                    std::array<NewtonWorldConvexCastReturnInfo, MaximumContactCount> currentContactList;
                    int contactCount = NewtonWorldConvexCast(newtonWorld, matrix.data, castTargetPosition.data, newtonUpperBodyShape, &impactTime, &castFilterData, ConvexCastFilter::PreFilter, currentContactList.data(), MaximumContactCount, threadHandle);
                    if (contactCount)
                    {
                        matrix.translation.xyz += velocity * (impactTime * frameTime);
                        if (impactTime > 0.0f)
                        {
                            matrix.translation.xyz -= velocity * (ContactSkinThickness / velocity.getLength());
                        }

                        normalizedTimeLeft -= impactTime;
                        float contactSpeedList[MaximumContactCount * 2];
                        float contactBounceSpeedList[MaximumContactCount * 2];
                        Math::Float3 contactBounceNormalList[MaximumContactCount * 2];
                        for (int contact = 1; contact < contactCount; ++contact)
                        {
                            Math::Float3 sourceNormal(currentContactList[contact - 1].m_normal);
                            for (int search = 0; search < contact; ++search)
                            {
                                Math::Float3 targetNormal(currentContactList[search].m_normal);
                                if (sourceNormal.dot(targetNormal) > (1.0f - NewtonEpsilon))
                                {
                                    currentContactList[contact] = currentContactList[contactCount - 1];
                                    contact--;
                                    contactCount--;
                                    break;
                                }
                            }
                        }

                        int currentContact = 0;
                        if (touchingSurface)
                        {
                            upConstraint.m_point[0] = matrix.translation.x;
                            upConstraint.m_point[1] = matrix.translation.y;
                            upConstraint.m_point[2] = matrix.translation.z;
                            upConstraint.m_point[3] = matrix.translation.w;

                            contactSpeedList[currentContact] = 0.0f;
                            contactBounceNormalList[currentContact].set(upConstraint.m_normal);
                            contactBounceSpeedList[currentContact] = calculateContactKinematics(velocity, &upConstraint);
                            currentContact++;
                        }

                        for (int contact = 0; contact < contactCount; ++contact)
                        {
                            contactSpeedList[currentContact] = 0.0f;
                            contactBounceNormalList[currentContact].set(currentContactList[contact].m_normal);
                            contactBounceSpeedList[currentContact] = calculateContactKinematics(velocity, &currentContactList[contact]);
                            currentContact++;
                        }

                        for (int contact = 0; contact < prevContactCount; ++contact)
                        {
                            contactSpeedList[currentContact] = 0.0f;
                            contactBounceNormalList[currentContact].set(previousContactList[contact].m_normal);
                            contactBounceSpeedList[currentContact] = calculateContactKinematics(velocity, &previousContactList[contact]);
                            currentContact++;
                        }

                        float residual = 10.0f;
                        Math::Float3 auxBounceVeloc(0.0f, 0.0f, 0.0f);
                        for (int contact = 0; (contact < MaximumSolverStepCount) && (residual > NewtonEpsilon); ++contact)
                        {
                            residual = 0.0f;
                            for (int search = 0; search < currentContact; ++search)
                            {
                                Math::Float3 normal(contactBounceNormalList[search]);
                                float v = contactBounceSpeedList[search] - normal.dot(auxBounceVeloc);
                                float x = contactSpeedList[search] + v;
                                if (x < 0.0f)
                                {
                                    v = 0.0f;
                                    x = 0.0f;
                                }

                                if (std::abs(v) > residual)
                                {
                                    residual = std::abs(v);
                                }

                                auxBounceVeloc += normal * (x - contactSpeedList[search]);
                                contactSpeedList[search] = x;
                            }
                        }

                        Math::Float3 stepVelocity(0.0f);
                        for (int contact = 0; contact < currentContact; ++contact)
                        {
                            Math::Float3 normal(contactBounceNormalList[contact]);
                            stepVelocity += normal * (contactSpeedList[contact]);
                        }

                        velocity += stepVelocity;
                        float velocityMagnitude = stepVelocity.getMagnitude();
                        if (velocityMagnitude < NewtonEpsilon)
                        {
                            float advanceTime = std::min(descreteTimeStep, normalizedTimeLeft * frameTime);
                            matrix.translation.xyz += (velocity * advanceTime);
                            normalizedTimeLeft -= advanceTime / frameTime;
                        }

                        prevContactCount = contactCount;
                        std::copy(std::begin(currentContactList), std::next(std::begin(currentContactList), contactCount), std::begin(previousContactList));

                    }
                    else
                    {
                        matrix.translation.xyz = castTargetPosition;
                        break;
                    }
                }

                NewtonCollisionSetScale(newtonUpperBodyShape, scale.x, scale.y, scale.z);

                // determine if player is standing on some plane
                Math::Float4x4 supportMatrix(matrix);
                supportMatrix.translation.xyz += (playerYAxis * sphereCastOrigin);
                if (!touchingSurface)
                {
                    Math::Float3 targetPoint(matrix.translation.xyz);
                    updateGroundPlane(matrix, supportMatrix, targetPoint, threadHandle);
                }
                else
                {
                    float timeStep = std::abs(playerYAxis.dot(velocity * frameTime));
                    float castDistance = (groundNormal.dot(groundNormal) > 0.0f) ? playerComponent.stairStep : timeStep;
                    Math::Float3 targetPoint(matrix.translation.xyz - (playerYAxis * (castDistance * 2.0f)));
                    updateGroundPlane(matrix, supportMatrix, targetPoint, threadHandle);
                }

                // set player velocity, position and orientation
                NewtonBodySetVelocity(newtonBody, velocity.data);
                NewtonBodySetMatrix(newtonBody, matrix.data);

                auto &transformComponent = entity->getComponent<Components::Transform>();
                transformComponent.position = (matrix.translation.xyz + (matrix.ry.xyz * playerComponent.height));
                transformComponent.rotation = (Math::Quaternion::MakePitchRotation(lookingAngle) * matrix.getRotation());
                forwardSpeed = 0.0f;
                lateralSpeed = 0.0f;
                verticalSpeed = 0.0f;
            }
		};

		StatePtr IdleState::onUpdate(PlayerBody *player, float frameTime)
		{
			return nullptr;
		}

		StatePtr IdleState::onAction(PlayerBody *player, Plugin::Population::Action const &action)
		{
			if (action.name == "crouch" && action.state)
			{
			}
			else if (action.name == "move_forward" && action.state)
			{
				return std::make_unique<WalkingState>();
			}
			else if (action.name == "move_backward" && action.state)
			{
				return std::make_unique<WalkingState>();
			}
			else if (action.name == "strafe_left" && action.state)
			{
				return std::make_unique<WalkingState>();
			}
			else if (action.name == "strafe_right" && action.state)
			{
				return std::make_unique<WalkingState>();
			}
			else if (action.name == "jump" && action.state && player->touchingSurface)
			{
				return std::make_unique<JumpingState>();
			}

			return nullptr;
		}

		StatePtr WalkingState::onUpdate(PlayerBody *player, float frameTime)
		{
			if (!player->moveForward && !player->moveBackward && !player->strafeLeft && !player->strafeRight)
			{
				return std::make_unique<IdleState>();
			}

			player->forwardSpeed += (((player->moveForward ? 1.0f : 0.0f) + (player->moveBackward ? -1.0f : 0.0f)) * 5.0f);
			player->lateralSpeed += (((player->strafeLeft ? -1.0f : 0.0f) + (player->strafeRight ? 1.0f : 0.0f)) * 5.0f);
			return nullptr;
		}

		StatePtr WalkingState::onAction(PlayerBody *player, Plugin::Population::Action const &action)
		{
			if (action.name == "jump" && action.state && player->touchingSurface)
			{
				return std::make_unique<JumpingState>();
			}

			return nullptr;
		}

		void JumpingState::onEnter(PlayerBody *player)
		{
            player->verticalSpeed += 20.0f;
            player->touchingSurface = false;
		}

		StatePtr JumpingState::onUpdate(PlayerBody *player, float frameTime)
		{
			if (player->touchingSurface)
			{
                if (player->moveForward || player->moveBackward || player->strafeLeft || player->strafeRight)
                {
                    return std::make_unique<WalkingState>();
                }
                else
                {
                    return std::make_unique<IdleState>();
                }
            }

			return nullptr;
		}

        StatePtr JumpingState::onAction(PlayerBody *player, Plugin::Population::Action const &action)
        {
            if (action.name == "jump" && action.state && player->touchingSurface)
            {
                return std::make_unique<JumpingState>();
            }

            return nullptr;
        }

        EntityPtr createPlayerBody(Plugin::Core *core, Plugin::Population *population, NewtonWorld *newtonWorld, Plugin::Entity * const entity)
		{
			return std::make_unique<PlayerBody>(core, population, newtonWorld, entity);
		}
	}; // namespace Newton
}; // namespace Gek