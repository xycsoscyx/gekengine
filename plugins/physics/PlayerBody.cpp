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

            void setRestrainingDistance(float distance)
            {
                restrainingDistance = std::max(std::abs(distance), PLAYER_MIN_RESTRAINING_DISTANCE);
            }

            void setClimbSlope(float slopeInRadians)
            {
                maximumSlope = std::cos(std::abs(slopeInRadians));
            }

            Math::Float3 calculateAverageOmega(const Math::Quaternion &rotation0, const Math::Quaternion &rotation1, float inverseFrameTime) const
            {
                float scale = 1.0f;
                if (rotation0.dot(rotation1) < 0.0f)
                {
                    scale = -1.0f;
                }

                Math::Quaternion delta((rotation0 * scale).getInverse() * rotation1);
                float deltaMagnitude = delta.axis.dot(delta.axis);
                if (deltaMagnitude	< (1.0e-5f * 1.0e-5f))
                {
                    return Math::Float3::Zero;
                }

                float inverseDeltaMagnitude = 1.0f / std::sqrt(deltaMagnitude);
                float normalizedMagnitude = deltaMagnitude * inverseDeltaMagnitude;

                float omegaMag = 2.0f * std::atan2(normalizedMagnitude, delta.angle) * inverseFrameTime;
                return delta.axis * (inverseDeltaMagnitude * omegaMag);
            }

            Math::Quaternion integrateOmega(const Math::Quaternion &rotation, const Math::Float3 &omega, float frameTime) const
            {
                // this is correct
                Math::Quaternion result(rotation);
                float omegaMagnitude = omega.dot(omega);
                const float ErrorAngle = 0.0125f * 3.141592f / 180.0f;
                const float ErrorAngleSquared = ErrorAngle * ErrorAngle;
                if (omegaMagnitude > ErrorAngleSquared)
                {
                    float inverseOmegaMagnitude = 1.0f / std::sqrt(omegaMagnitude);
                    Math::Float3 omegaAxis(omega * (inverseOmegaMagnitude));
                    float omegaAngle = inverseOmegaMagnitude * omegaMagnitude * frameTime;
                    Math::Quaternion deltaRotation(Math::Quaternion::FromAngular(omegaAxis, omegaAngle));
                    result = (result * deltaRotation).getNormal();
                }

                return result;
            }

            Math::Float3 CalculateDesiredOmega(float frameTime) const
            {
                Math::Quaternion newtonRotation;
                NewtonBodyGetRotation(newtonBody, newtonRotation.data);
                Math::Quaternion playerRotation(newtonRotation[1], newtonRotation[2], newtonRotation[3], newtonRotation[0]);

                Math::Quaternion targetRotation(Math::Quaternion::FromYaw(headingAngle));

                return calculateAverageOmega(playerRotation, targetRotation, 0.5f / frameTime);
            }

            Math::Float3 CalculateDesiredVelocity(const Math::Float3& gravity, float frameTime) const
            {
                Math::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);

                Math::Float3 updir(matrix.ry.xyz);
                Math::Float3 frontDir(matrix.rz.xyz);
                Math::Float3 rightDir(frontDir.cross(updir));

                Math::Float3 veloc;
                if ((verticalSpeed <= 0.0f) && (groundNormal.dot(groundNormal)) > 0.0f)
                {
                    // plane is supported by a ground plane, apply the player input velocity
                    if (groundNormal.dot(updir) >= maximumSlope)
                    {
                        // player is in a legal slope, he is in full control of his movement
                        Math::Float3 bodyVeloc;
                        NewtonBodyGetVelocity(newtonBody, bodyVeloc.data);
                        veloc = (updir * bodyVeloc.dot(updir)) + (gravity * frameTime) + (frontDir * forwardSpeed) + (rightDir * lateralSpeed) + (updir * verticalSpeed);
                        veloc += (groundVelocity - (updir * updir.dot(groundVelocity)));

                        float speedLimitMag2 = forwardSpeed * forwardSpeed + lateralSpeed * lateralSpeed + verticalSpeed * verticalSpeed + groundVelocity.dot(groundVelocity) + 0.1f;
                        float speedMag2 = veloc.dot(veloc);
                        if (speedMag2 > speedLimitMag2)
                        {
                            veloc = (veloc * std::sqrt(speedLimitMag2 / speedMag2));
                        }

                        float normalVeloc = groundNormal.dot(veloc - groundVelocity);
                        if (normalVeloc < 0.0f)
                        {
                            veloc -= (groundNormal * normalVeloc);
                        }
                    }
                    else
                    {
                        // player is in an illegal ramp, he slides down hill an loses control of his movement 
                        NewtonBodyGetVelocity(newtonBody, veloc.data);
                        veloc += (updir * verticalSpeed);
                        veloc += (gravity * frameTime);
                        float normalVeloc = groundNormal.dot(veloc - groundVelocity);
                        if (normalVeloc < 0.0f)
                        {
                            veloc -= (groundNormal * normalVeloc);
                        }
                    }
                }
                else
                {
                    // player is on free fall, only apply the gravity
                    NewtonBodyGetVelocity(newtonBody, veloc.data);
                    veloc += (updir * verticalSpeed);
                    veloc += (gravity * frameTime);
                }

                return veloc;
            }

            void SetPlayerVelocity(const Math::Float3& gravity, float frameTime)
            {
                Math::Float3 omega(CalculateDesiredOmega(frameTime));
                NewtonBodySetOmega(newtonBody, omega.data);

                Math::Float3 veloc(CalculateDesiredVelocity(gravity, frameTime));
                NewtonBodySetVelocity(newtonBody, veloc.data);
            }

            float CalculateContactKinematics(const Math::Float3& veloc, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
            {
                Math::Float3 contactVeloc(0.0f, 0.0f, 0.0f);
                if (contactInfo->m_hitBody)
                {
                    NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, contactVeloc.data);
                }

                const float restitution = 0.0f;
                Math::Float3 normal(contactInfo->m_normal);
                float reboundVelocMag = -(veloc - contactVeloc).dot(normal) * (1.0f + restitution);
                return (reboundVelocMag > 0.0f) ? reboundVelocMag : 0.0f;
            }

            void UpdateGroundPlane(Math::Float4x4& matrix, const Math::Float4x4& castMatrix, const Math::Float3& dst, int threadHandle)
            {
                groundNormal = Math::Float3(0.0f);
                groundVelocity = Math::Float3(0.0f);

                float param = 10.0f;
                ConvexCastFilter filter(newtonBody);
                NewtonWorldConvexCastReturnInfo info;
                int count = NewtonWorldConvexCast(newtonWorld, castMatrix.data, dst.data, newtonCastingShape, &param, &filter, ConvexCastFilter::PreFilter, &info, 1, threadHandle);
                if (touchingSurface = (count && (param <= 1.0f)))
                {
                    Math::Float3 supportPoint(castMatrix.translation.xyz + ((dst - castMatrix.translation.xyz) * param));
                    groundNormal.set(info.m_normal);
                    NewtonBodyGetPointVelocity(info.m_hitBody, supportPoint.data, groundVelocity.data);
                    matrix.translation.xyz = supportPoint;
                    matrix.translation.w = 1.0f;
                }
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

                Math::Float3 position;
                NewtonBodyGetPosition(newtonBody, position.data);
                auto gravity(world->getGravity(position));
                SetPlayerVelocity(gravity, frameTime);
            }

            void onPostUpdate(float frameTime, int threadHandle)
			{
                const auto &playerComponent = entity->getComponent<Components::Player>();

                Math::Float4x4 matrix;
                NewtonBodyGetMatrix(newtonBody, matrix.data);

                Math::Float3 veloc;
                NewtonBodyGetVelocity(newtonBody, veloc.data);

                Math::Float3 omega;
                NewtonBodyGetOmega(newtonBody, omega.data);

                // integrate body angular velocity
                Math::Quaternion newtonRotation;
                NewtonBodyGetRotation(newtonBody, newtonRotation.data);
                Math::Quaternion bodyRotation(newtonRotation[1], newtonRotation[2], newtonRotation[3], newtonRotation[0]);
                bodyRotation = integrateOmega(bodyRotation, omega, frameTime);
                matrix.setRotation(bodyRotation);

                // integrate linear velocity
                float normalizedTimeLeft = 1.0f;
                float step = frameTime * std::sqrt(veloc.dot(veloc));
                float descreteTimeStep = frameTime * (1.0f / D_DESCRETE_MOTION_STEPS);
                int prevContactCount = 0;
                ConvexCastFilter castFilterData(newtonBody);
                NewtonWorldConvexCastReturnInfo prevInfo[PLAYER_CONTROLLER_MAX_CONTACTS];

                Math::Float3 updir(matrix.ry.xyz);

                Math::Float3 scale;
                NewtonCollisionGetScale(newtonUpperBodyShape, &scale.x, &scale.y, &scale.z);

                //const float radio = m_outerRadio * 4.0f;
                const float radio = (playerComponent.outerRadius + restrainingDistance) * 4.0f;
                NewtonCollisionSetScale(newtonUpperBodyShape, playerComponent.height - playerComponent.stairStep, radio, radio);

                Math::Float3 worldUpDirection(0.0f, 1.0f, 0.0f);
                NewtonWorldConvexCastReturnInfo upConstratint;
                memset(&upConstratint, 0, sizeof(upConstratint));
                upConstratint.m_normal[0] = worldUpDirection.x;
                upConstratint.m_normal[1] = worldUpDirection.y;
                upConstratint.m_normal[2] = worldUpDirection.z;
                upConstratint.m_normal[3] = 0.0f;

                for (int j = 0; (j < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedTimeLeft > 1.0e-5f); j++)
                {
                    if (veloc.dot(veloc) < 1.0e-6f)
                    {
                        break;
                    }

                    float timetoImpact;
                    NewtonWorldConvexCastReturnInfo info[PLAYER_CONTROLLER_MAX_CONTACTS];
                    Math::Float3 destPosit(matrix.translation.xyz + (veloc * frameTime));
                    int contactCount = NewtonWorldConvexCast(newtonWorld, matrix.data, destPosit.data, newtonUpperBodyShape, &timetoImpact, &castFilterData, ConvexCastFilter::PreFilter, info, PLAYER_CONTROLLER_MAX_CONTACTS, threadHandle);
                    if (contactCount)
                    {
                        //contactCount = manager->ProcessContacts(this, info, contactCount);
                    }

                    if (contactCount)
                    {
                        matrix.translation.xyz += veloc * (timetoImpact * frameTime);
                        if (timetoImpact > 0.0f)
                        {
                            matrix.translation.xyz -= veloc * (D_PLAYER_CONTACT_SKIN_THICKNESS / std::sqrt(veloc.dot(veloc)));
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
                            upConstratint.m_point[3] = matrix.translation.w;

                            speed[count] = 0.0f;
                            bounceNormal[count].set(upConstratint.m_normal);
                            bounceSpeed[count] = CalculateContactKinematics(veloc, &upConstratint);
                            count++;
                        }

                        for (int i = 0; i < contactCount; i++)
                        {
                            speed[count] = 0.0f;
                            bounceNormal[count].set(info[i].m_normal);
                            bounceSpeed[count] = CalculateContactKinematics(veloc, &info[i]);
                            count++;
                        }

                        for (int i = 0; i < prevContactCount; i++)
                        {
                            speed[count] = 0.0f;
                            bounceNormal[count].set(prevInfo[i].m_normal);
                            bounceSpeed[count] = CalculateContactKinematics(veloc, &prevInfo[i]);
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
                            velocStep += normal * (speed[i]);
                        }

                        veloc += velocStep;
                        float velocMag2 = velocStep.dot(velocStep);
                        if (velocMag2 < 1.0e-6f)
                        {
                            float advanceTime = std::min(descreteTimeStep, normalizedTimeLeft * frameTime);
                            matrix.translation.xyz += veloc * (advanceTime);
                            normalizedTimeLeft -= advanceTime / frameTime;
                        }

                        prevContactCount = contactCount;
                        std::copy(std::begin(info), std::end(info), prevInfo);
                    }
                    else
                    {
                        matrix.translation.xyz = destPosit;
                        matrix.translation.w = 1.0f;
                        break;
                    }
                }

                NewtonCollisionSetScale(newtonUpperBodyShape, scale.x, scale.y, scale.z);

                // determine if player is standing on some plane
                Math::Float4x4 supportMatrix(matrix);
                supportMatrix.translation.xyz += (updir * sphereCastOrigin);
                if (!touchingSurface)
                {
                    Math::Float3 dst(matrix.translation);
                    UpdateGroundPlane(matrix, supportMatrix, dst, threadHandle);
                }
                else
                {
                    step = std::abs(updir.dot(veloc * frameTime));
                    float castDist = (groundNormal.dot(groundNormal) > 0.0f) ? playerComponent.stairStep : step;
                    Math::Float3 dst(matrix.translation - (updir * (castDist * 2.0f)));
                    UpdateGroundPlane(matrix, supportMatrix, dst, threadHandle);
                }

                // set player velocity, position and orientation
                NewtonBodySetVelocity(newtonBody, veloc.data);
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
			player->touchingSurface = false;
		}

		void JumpingState::onExit(PlayerBody *player)
		{
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