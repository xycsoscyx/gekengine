#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\World.h"
#include "GEK\Newton\Entity.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Newton\PlayerBody.h"
#include <Newton.h>
#include <algorithm>
#include <memory>
#include <stack>

static const float PLAYER_MIN_RESTRAINING_DISTANCE = 1.0e-2f;
static const float PLAYER_EPSILON = 1.0e-5f;
static const float PLAYER_EPSILON_SQUARED = (PLAYER_EPSILON * PLAYER_EPSILON);

static const int D_PLAYER_CONTROLLER_MAX_CONTACTS = 32;
static const int D_PLAYER_DESCRETE_MOTION_STEPS = 8;
static const int D_PLAYER_MAX_INTERGRATION_STEPS = 8;
static const int D_PLAYER_MAX_SOLVER_ITERATIONS = 16;
static const float D_PLAYER_CONTACT_SKIN_THICKNESS = 0.025f;

namespace Gek
{
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

    class PlayerNewtonBody;

    GEK_INTERFACE(State)
    {
        virtual void onEnter(PlayerNewtonBody *player) { };
        virtual void onExit(PlayerNewtonBody *player) { };

        virtual StatePtr onUpdate(PlayerNewtonBody *player, float frameTime) { return nullptr; };
        virtual StatePtr onAction(PlayerNewtonBody *player, const wchar_t *name, const Plugin::ActionState &state) { return nullptr; };
    };

    class IdleState
        : public State
    {
    public:
        StatePtr onUpdate(PlayerNewtonBody *player, float frameTime);
        StatePtr onAction(PlayerNewtonBody *player, const wchar_t *name, const Plugin::ActionState &state);
    };

    class CrouchingState
        : public State
    {
    public:
        StatePtr onUpdate(PlayerNewtonBody *player, float frameTime);
        StatePtr onAction(PlayerNewtonBody *player, const wchar_t *name, const Plugin::ActionState &state);
    };

    class WalkingState
        : public State
    {
    public:
        StatePtr onUpdate(PlayerNewtonBody *player, float frameTime);
        StatePtr onAction(PlayerNewtonBody *player, const wchar_t *name, const Plugin::ActionState &state);
    };

    class JumpingState
        : public State
    {
    public:
        void onEnter(PlayerNewtonBody *player);
        void onExit(PlayerNewtonBody *player);
        StatePtr onUpdate(PlayerNewtonBody *player, float frameTime);
    };

    class FallingState
        : public State
    {
    private:
        float time;

    public:
        void onEnter(PlayerNewtonBody *player);
        StatePtr onUpdate(PlayerNewtonBody *player, float frameTime);
    };

    class DroppingState
        : public State
    {
    public:
        StatePtr onUpdate(PlayerNewtonBody *player, float frameTime);
    };

    class PlayerNewtonBody
        : public Plugin::CoreObserver
        , public Newton::Entity
    {
    public:
        Plugin::Core *core;
        Newton::World *world;
        NewtonWorld *newtonWorld;

        Plugin::Entity *entity;
        float halfHeight;
        float maximumSlope;
        float restrainingDistance;
        float sphereCastOrigin;

        NewtonBody *newtonBody;
        NewtonCollision *newtonCastingShape;
        NewtonCollision *newtonSupportShape;
        NewtonCollision *newtonUpperBodyShape;

        StatePtr currentState;
        bool moveForward;
        bool moveBackward;
        bool strafeLeft;
        bool strafeRight;
        float headingAngle;
        float forwardSpeed;
        float lateralSpeed;
        float verticalSpeed;

        Math::Float3 groundNormal;
        Math::Float3 groundVelocity;

        bool jumping;
        bool crouching;
        bool touchingSurface;
        bool falling;

    public:
        PlayerNewtonBody(Plugin::Core *core,
            NewtonWorld *newtonWorld,
            Plugin::Entity *entity)
            : core(core)
            , world(static_cast<Newton::World *>(NewtonWorldGetUserData(newtonWorld)))
            , newtonWorld(newtonWorld)
            , newtonBody(nullptr)
            , entity(entity)
            , currentState(std::make_shared<IdleState>())
            , halfHeight(0.0f)
            , moveForward(false)
            , moveBackward(false)
            , strafeLeft(false)
            , strafeRight(false)
            , headingAngle(0.0f)
            , forwardSpeed(0.0f)
            , lateralSpeed(0.0f)
            , verticalSpeed(0.0f)
            , groundNormal(0.0f)
            , groundVelocity(0.0f)
            , jumping(false)
            , crouching(false)
            , touchingSurface(false)
            , falling(true)
        {
            auto &mass = entity->getComponent<Components::Mass>();
            auto &transform = entity->getComponent<Components::Transform>();
            auto &player = entity->getComponent<Components::PlayerBody>();

            halfHeight = player.height * 0.5f;

            setRestrainingDistance(0.01f);

            setClimbSlope(45.0f * 3.1416f / 180.0f);

            const int numberOfSteps = 12;
            Math::Float3 convexPoints[2][numberOfSteps];

            // create an inner thin cylinder
            Math::Float3 point0(0.0f, player.innerRadius, 0.0f);
            Math::Float3 point1(halfHeight, player.innerRadius, 0.0f);
            for (int currentPoint = 0; currentPoint < numberOfSteps; currentPoint++)
            {
                Math::Float4x4 rotation;
                rotation.setPitchRotation(currentPoint * 2.0f * 3.141592f / numberOfSteps);
                convexPoints[0][currentPoint] = (rotation * point0);
                convexPoints[1][currentPoint] = (rotation * point1);
            }

            NewtonCollision* const supportShape = NewtonCreateConvexHull(newtonWorld, (numberOfSteps * 2), convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, nullptr);

            // create the outer thick cylinder
            Math::Float4x4 outerShapeMatrix(Math::Float4x4::Identity);
            float capsuleHeight = (halfHeight - player.stairStep);
            sphereCastOrigin = ((capsuleHeight * 0.5f) + player.stairStep);
            outerShapeMatrix.translation = (outerShapeMatrix.ny * sphereCastOrigin);
            NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.25f, 0.5f, 0, outerShapeMatrix.data);
            NewtonCollisionSetScale(bodyCapsule, capsuleHeight, (player.outerRadius * 4.0f), (player.outerRadius * 4.0f));

            // compound collision player controller
            NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
            NewtonCompoundCollisionBeginAddRemove(playerShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
            NewtonCompoundCollisionEndAddRemove(playerShape);

            // create the kinematic body
            Math::Float4x4 matrix(transform.getMatrix());
            matrix.translation -= (matrix.ny * player.height);
            newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, matrix.data);
            NewtonBodySetUserData(newtonBody, dynamic_cast<Newton::Entity *>(this));

            // players must have weight, otherwise they are infinitely strong when they collide
            NewtonCollision* const bodyShape = NewtonBodyGetCollision(newtonBody);
            NewtonBodySetMassProperties(newtonBody, mass.value, bodyShape);

            // make the body collidable with other dynamics bodies, by default
            NewtonBodySetCollidable(newtonBody, true);

            float castHeight = (capsuleHeight * 0.4f);
            float castRadius = std::max(0.5f, (player.innerRadius * 0.5f));

            point0.set(0.0f, castRadius, 0.0f);
            point1.set(castHeight, castRadius, 0.0f);
            for (int currentPoint = 0; currentPoint < numberOfSteps; currentPoint++)
            {
                Math::Float4x4 rotation;
                rotation.setPitchRotation(currentPoint * 2.0f * Math::Pi / numberOfSteps);
                convexPoints[0][currentPoint] = (rotation * point0);
                convexPoints[1][currentPoint] = (rotation * point1);
            }

            newtonCastingShape = NewtonCreateConvexHull(newtonWorld, (numberOfSteps * 2), convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, nullptr);
            newtonSupportShape = NewtonCompoundCollisionGetCollisionFromNode(bodyShape, NewtonCompoundCollisionGetNodeByIndex(bodyShape, 0));
            newtonUpperBodyShape = NewtonCompoundCollisionGetCollisionFromNode(bodyShape, NewtonCompoundCollisionGetNodeByIndex(bodyShape, 1));

            NewtonDestroyCollision(bodyCapsule);
            NewtonDestroyCollision(supportShape);
            NewtonDestroyCollision(playerShape);
        }

        ~PlayerNewtonBody(void)
        {
            NewtonDestroyCollision(newtonCastingShape);
            NewtonDestroyCollision(newtonSupportShape);
            NewtonDestroyCollision(newtonUpperBodyShape);
            core->removeObserver((Plugin::CoreObserver *)this);
        }

        // Plugin::CoreObserver
        void onAction(const wchar_t *name, const Plugin::ActionState &state)
        {
            if (_wcsicmp(name, L"turn") == 0)
            {
                headingAngle += (state.value * 0.01f);
            }
            else if (_wcsicmp(name, L"move_forward") == 0)
            {
                moveForward = state.state;
            }
            else if (_wcsicmp(name, L"move_backward") == 0)
            {
                moveBackward = state.state;
            }
            else if (_wcsicmp(name, L"strafe_left") == 0)
            {
                strafeLeft = state.state;
            }
            else if (_wcsicmp(name, L"strafe_right") == 0)
            {
                strafeRight = state.state;
            }
            else if (_wcsicmp(name, L"crouch") == 0)
            {
                crouching = state.state;
            }

            StatePtr nextState(currentState->onAction(this, name, state));
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
            StatePtr nextState(currentState->onUpdate(this, frameTime));
            if (nextState)
            {
                currentState->onExit(this);
                nextState->onEnter(this);
                currentState = nextState;
            }

            float rotation[4];
            NewtonBodyGetRotation(newtonBody, rotation);
            setDesiredOmega(Math::Quaternion(rotation[1], rotation[2], rotation[3], rotation[0]), frameTime);

            Math::Float4x4 matrix;
            NewtonBodyGetMatrix(newtonBody, matrix.data);
            setDesiredVelocity(matrix, frameTime);
        }

        void onPostUpdate(float frameTime, int threadHandle)
        {
            auto &player = entity->getComponent<Components::PlayerBody>();

            // get the body motion state 
            Math::Float4x4 matrix;
            NewtonBodyGetMatrix(newtonBody, matrix.data);

            Math::Float3 gravity(world->getGravity(matrix.translation));

            Math::Float3 omega;
            NewtonBodyGetOmega(newtonBody, omega.data);

            // integrate body angular velocity
            Math::Quaternion bodyRotation(integrateOmega(matrix.getQuaternion(), omega, frameTime));
            matrix = bodyRotation.getMatrix(matrix.translation);

            // integrate linear velocity
            Math::Float3 velocity;
            NewtonBodyGetVelocity(newtonBody, velocity.data);
            float normalizedTimeLeft = 1.0f;
            float velocityStep = (frameTime * velocity.getLength());
            float descreteframeTime = (frameTime * (1.0f / D_PLAYER_DESCRETE_MOTION_STEPS));
            int previousContactCount = 0;
            ConvexCastPreFilter preFilterData(newtonBody);
            NewtonWorldConvexCastReturnInfo previousInfoList[D_PLAYER_CONTROLLER_MAX_CONTACTS];

            Math::Float3 scale;
            NewtonCollisionGetScale(newtonUpperBodyShape, &scale.x, &scale.y, &scale.z);

            //const float radius = (player.outerRadius * 4.0f);
            const float radius = ((player.outerRadius + restrainingDistance) * 4.0f);
            NewtonCollisionSetScale(newtonUpperBodyShape, (halfHeight - player.stairStep), radius, radius);

            NewtonWorldConvexCastReturnInfo upConstraint;
            memset(&upConstraint, 0, sizeof(upConstraint));
            upConstraint.m_normal[0] = -gravity.x;
            upConstraint.m_normal[1] = -gravity.y;
            upConstraint.m_normal[2] = -gravity.z;

            for (int currentStep = 0; ((currentStep < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedTimeLeft > PLAYER_EPSILON)); currentStep++)
            {
                if (velocity.getLengthSquared() < PLAYER_EPSILON)
                {
                    break;
                }

                float timeToImpact = 0.0f;
                NewtonWorldConvexCastReturnInfo currentInfoList[D_PLAYER_CONTROLLER_MAX_CONTACTS];
                Math::Float3 destinationPosition(matrix.translation + (velocity * frameTime));
                int contactCount = NewtonWorldConvexCast(newtonWorld, matrix.data, destinationPosition.data, newtonUpperBodyShape, &timeToImpact, &preFilterData, ConvexCastPreFilter::preFilter, currentInfoList, ARRAYSIZE(currentInfoList), threadHandle);
                if (contactCount)
                {
                    matrix.translation += (velocity * (timeToImpact * frameTime));
                    if (timeToImpact > 0.0f)
                    {
                        matrix.translation -= (velocity * (D_PLAYER_CONTACT_SKIN_THICKNESS / velocity.getLength()));
                    }

                    normalizedTimeLeft -= timeToImpact;

                    float speedDeltaList[D_PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                    float bounceSpeedList[D_PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                    Math::Float3 bounceNormalList[D_PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                    for (int currentContact = 1; currentContact < contactCount; currentContact++)
                    {
                        Math::Float3 normal0(currentInfoList[currentContact - 1].m_normal);
                        for (int previousContact = 0; previousContact < currentContact; previousContact++)
                        {
                            Math::Float3 normal1(currentInfoList[previousContact].m_normal);
                            if (normal0.dot(normal1) > 0.9999f)
                            {
                                currentInfoList[currentContact] = currentInfoList[contactCount - 1];
                                currentContact--;
                                contactCount--;
                                break;
                            }
                        }
                    }

                    int bounceCount = 0;
                    auto setBounceData = [&](const NewtonWorldConvexCastReturnInfo &castInfo) -> void
                    {
                        speedDeltaList[bounceCount] = 0.0f;
                        bounceNormalList[bounceCount] = castInfo.m_normal;
                        bounceSpeedList[bounceCount] = calculateContactKinematics(velocity, &castInfo);
                        bounceCount++;
                    };

                    upConstraint.m_point[0] = matrix.translation.x;
                    upConstraint.m_point[1] = matrix.translation.y;
                    upConstraint.m_point[2] = matrix.translation.z;
                    setBounceData(upConstraint);

                    for (int currentContact = 0; currentContact < contactCount; currentContact++)
                    {
                        setBounceData(currentInfoList[currentContact]);
                    }

                    for (int currentContact = 0; currentContact < previousContactCount; currentContact++)
                    {
                        setBounceData(previousInfoList[currentContact]);
                    }

                    float residualSpeed = 10.0f;
                    Math::Float3 auxiliaryBounceVelocity(0.0f);
                    for (int currentContact = 0; ((currentContact < D_PLAYER_MAX_SOLVER_ITERATIONS) && (residualSpeed > PLAYER_EPSILON)); currentContact++)
                    {
                        residualSpeed = 0.0f;
                        for (int currentBound = 0; currentBound < bounceCount; currentBound++)
                        {
                            Math::Float3 normal(bounceNormalList[currentBound]);
                            float currentSpeed = (bounceSpeedList[currentBound] - normal.dot(auxiliaryBounceVelocity));
                            float speedDelta = (speedDeltaList[currentBound] + currentSpeed);
                            if (speedDelta < 0.0f)
                            {
                                currentSpeed = 0.0f;
                                speedDelta = 0.0f;
                            }

                            residualSpeed = std::max(std::abs(currentSpeed), residualSpeed);
                            auxiliaryBounceVelocity += (normal * (speedDelta - speedDeltaList[currentBound]));
                            speedDeltaList[currentBound] = speedDelta;
                        }
                    }

                    Math::Float3 velocityStep(0.0f);
                    for (int currentContact = 0; currentContact < bounceCount; currentContact++)
                    {
                        Math::Float3 normal(bounceNormalList[currentContact]);
                        velocityStep += (normal * speedDeltaList[currentContact]);
                    }

                    velocity += velocityStep;
                    float velocityMagnitudeSquared = velocityStep.getLengthSquared();
                    if (velocityMagnitudeSquared < PLAYER_EPSILON_SQUARED)
                    {
                        float advanceTime = std::min(descreteframeTime, (normalizedTimeLeft * frameTime));
                        matrix.translation += (velocity * advanceTime);
                        normalizedTimeLeft -= (advanceTime / frameTime);
                    }

                    previousContactCount = contactCount;
                    memcpy(previousInfoList, currentInfoList, (previousContactCount * sizeof(NewtonWorldConvexCastReturnInfo)));
                }
                else
                {
                    matrix.translation = destinationPosition;
                    break;
                }
            }

            NewtonCollisionSetScale(newtonUpperBodyShape, scale.x, scale.y, scale.z);

            Math::Float4x4 supportMatrix(matrix);
            supportMatrix.translation += (matrix.ny * sphereCastOrigin);
            if (jumping || falling) // Allow jumping
            {
                Math::Float3 destination(matrix.translation);
                updateGroundPlane(matrix, supportMatrix, destination, threadHandle);
            }
            else // Snap to ground plane
            {
                velocityStep = std::abs(matrix.ny.dot(velocity * frameTime));
                float castDist = ((groundNormal.getLengthSquared() > 0.0f) ? player.stairStep : velocityStep);
                Math::Float3 destination(matrix.translation - (matrix.ny * (castDist * 2.0f)));
                updateGroundPlane(matrix, supportMatrix, destination, threadHandle);
            }

            if (!touchingSurface)
            {
                falling = (velocity.getNormal().dot(gravity.getNormal()) > 0.75f);
            }

            NewtonBodySetVelocity(newtonBody, velocity.data);
            NewtonBodySetMatrix(newtonBody, matrix.data);

            auto &transform = entity->getComponent<Components::Transform>();
            transform.position = (matrix.translation + (matrix.ny * player.height));
            transform.rotation = matrix.getQuaternion();
        }

    private:
        void setRestrainingDistance(float distance)
        {
            restrainingDistance = std::max(std::abs(distance), PLAYER_MIN_RESTRAINING_DISTANCE);
        }

        void setClimbSlope(float slopeInRadians)
        {
            maximumSlope = std::cos(std::abs(slopeInRadians));
        }

        Math::Quaternion integrateOmega(const Math::Quaternion &rotation, const Math::Float3 &omega, float frameTime) const
        {
            Math::Float3 theta(omega * frameTime * 0.5f);
            float thetaMagnitideSquared = theta.getLengthSquared();

            float angle;
            Math::Quaternion deltaRotation;
            if (thetaMagnitideSquared * thetaMagnitideSquared / 24.0f < Math::Epsilon)
            {
                deltaRotation.w = 1.0f - thetaMagnitideSquared / 2.0f;
                angle = 1.0f - thetaMagnitideSquared / 6.0f;
            }
            else
            {
                float thetaMag = sqrt(thetaMagnitideSquared);
                deltaRotation.w = cos(thetaMag);
                angle = sin(thetaMag) / thetaMag;
            }

            deltaRotation.x = theta.x * angle;
            deltaRotation.y = theta.y * angle;
            deltaRotation.z = theta.z * angle;
            return (deltaRotation * rotation);
        }

        void setDesiredOmega(const Math::Quaternion &rotation, float frameTime) const
        {
            Math::Quaternion quaternion0(rotation);
            Math::Quaternion quaternion1(Math::Float3(0.0f, 1.0f, 0.0f), headingAngle);
            if (quaternion0.dot(quaternion1) < 0.0f)
            {
                quaternion0 *= -1.0f;
            }

            Math::Quaternion deltaQuaternion(quaternion0.getInverse() * quaternion1);
            Math::Float3 omegaDirection(deltaQuaternion.x, deltaQuaternion.y, deltaQuaternion.z);
            float directionMagnitudeSquared = omegaDirection.getLengthSquared();
            if (directionMagnitudeSquared >= PLAYER_EPSILON_SQUARED)
            {
                float inverseDirectionMagnitude = (1.0f / std::sqrt(directionMagnitudeSquared));
                float directionMagnitude = (directionMagnitudeSquared * inverseDirectionMagnitude);
                float omegaMagnitude = (2.0f * std::atan2(directionMagnitude, deltaQuaternion.w) * (0.5f / frameTime));
                Math::Float3 desiredOmega(omegaDirection * (inverseDirectionMagnitude * omegaMagnitude));
                NewtonBodySetOmega(newtonBody, desiredOmega.data);
            }
        }

        void setDesiredVelocity(const Math::Float4x4 &matrix, float frameTime)
        {
            Math::Float3 gravity(world->getGravity(matrix.translation));

            Math::Float3 desiredVelocity;
            NewtonBodyGetVelocity(newtonBody, desiredVelocity.data);
            if ((verticalSpeed <= 0.0f) && (groundNormal.getLengthSquared() > 0.0f))
            {
                // plane is supported by a ground plane, apply the player input desiredVelocity
                if (groundNormal.dot(matrix.ny) >= maximumSlope)
                {
                    // player is in a legal slope, he is in full control of his movement
                    desiredVelocity = (gravity * frameTime);
                    desiredVelocity += (matrix.nz * forwardSpeed);
                    desiredVelocity += (matrix.nx * lateralSpeed);
                    desiredVelocity += (groundVelocity - (matrix.ny * matrix.ny.dot(groundVelocity)));
                    desiredVelocity += (matrix.ny * desiredVelocity.dot(matrix.ny));

                    float speedMagnitudeSquared = desiredVelocity.getLengthSquared();
                    float speedLimitMagnitudeSquared = ((forwardSpeed * forwardSpeed) + (lateralSpeed * lateralSpeed) + groundVelocity.getLengthSquared() + 0.1f);
                    if (speedMagnitudeSquared > speedLimitMagnitudeSquared)
                    {
                        desiredVelocity *= std::sqrt(speedLimitMagnitudeSquared / speedMagnitudeSquared);
                    }

                    float groundNormalVelocity = groundNormal.dot(desiredVelocity - groundVelocity);
                    if (groundNormalVelocity < 0.0f)
                    {
                        desiredVelocity -= (groundNormal * groundNormalVelocity);
                    }
                }
                else
                {
                    // player is in an illegal ramp, he slides down hill an loses control of his movement 
                    desiredVelocity += (gravity * frameTime);
                    float groundNormalVelocity = groundNormal.dot(desiredVelocity - groundVelocity);
                    if (groundNormalVelocity < 0.0f)
                    {
                        desiredVelocity -= (groundNormal * groundNormalVelocity);
                    }
                }
            }
            else
            {
                // player is on free fall, only apply the gravity
                desiredVelocity += (matrix.ny * verticalSpeed);
                desiredVelocity += (gravity * frameTime);
            }

            NewtonBodySetVelocity(newtonBody, desiredVelocity.data);
            forwardSpeed = 0.0f;
            lateralSpeed = 0.0f;
            verticalSpeed = 0.0f;
        }

        float calculateContactKinematics(const Math::Float3 &velocity, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
        {
            Math::Float3 contactVelocity;
            if (contactInfo->m_hitBody)
            {
                NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, contactVelocity.data);
            }
            else
            {
                contactVelocity.set(0.0f, 0.0f, 0.0f);
            }

            const float restitution = 0.0f;
            Math::Float3 normal(contactInfo->m_normal);
            float reboundVelocityMagnitude = -((velocity - contactVelocity).dot(normal) * (1.0f + restitution));
            return std::max(0.0f, reboundVelocityMagnitude);
        }

        void updateGroundPlane(Math::Float4x4 &matrix, const Math::Float4x4 &castMatrix, const Math::Float3 &destination, int threadHandle)
        {
            groundNormal.set(0.0f, 0.0f, 0.0f);
            groundVelocity.set(0.0f, 0.0f, 0.0f);

            float parameter = 10.0f;
            NewtonWorldConvexCastReturnInfo info;
            ConvexCastPreFilter preFilterData(newtonBody);
            if (NewtonWorldConvexCast(newtonWorld, castMatrix.data, destination.data, newtonCastingShape, &parameter, &preFilterData, ConvexCastPreFilter::preFilter, &info, 1, threadHandle) > 0 && parameter <= 1.0f)
            {
                touchingSurface = true;
                groundNormal.set(info.m_normal[0], info.m_normal[1], info.m_normal[2]);
                Math::Float3 supportPoint(castMatrix.translation + ((destination - castMatrix.translation) * parameter));
                NewtonBodyGetPointVelocity(info.m_hitBody, supportPoint.data, groundVelocity.data);
                matrix.translation = supportPoint;
            }
            else
            {
                touchingSurface = false;
            }
        }
    };

    Newton::EntityPtr createPlayerBody(Plugin::Core *core, NewtonWorld *newtonWorld, Plugin::Entity *entity)
    {
        std::shared_ptr<PlayerNewtonBody> player;
        try
        {
            player = std::make_shared<PlayerNewtonBody>(core, newtonWorld, entity);
        }
        catch (const std::bad_alloc &badAllocation)
        {
            GEK_THROW_EXCEPTION(Trace::Exception, "Unable to allocate new player object: %v", badAllocation.what());
        };

        core->addObserver((Plugin::CoreObserver *)player.get());
        return player;
    }

    /* Idle
        If the ground drops out, the player starts to drop (uncontrolled)
        Actions trigger action states (crouch, walk, jump)
    */
    StatePtr IdleState::onUpdate(PlayerNewtonBody *player, float frameTime)
    {
        if (player->falling)
        {
            return std::make_shared<DroppingState>();
        }

        return nullptr;
    }

    StatePtr IdleState::onAction(PlayerNewtonBody *player, const wchar_t *name, const Plugin::ActionState &state)
    {
        if (_wcsicmp(name, L"crouch") == 0 && state.state)
        {
            return std::make_shared<CrouchingState>();
        }
        else if (_wcsicmp(name, L"move_forward") == 0 && state.state)
        {
            return std::make_shared<WalkingState>();
        }
        else if (_wcsicmp(name, L"move_backward") == 0 && state.state)
        {
            return std::make_shared<WalkingState>();
        }
        else if (_wcsicmp(name, L"strafe_left") == 0 && state.state)
        {
            return std::make_shared<WalkingState>();
        }
        else if (_wcsicmp(name, L"strafe_right") == 0 && state.state)
        {
            return std::make_shared<WalkingState>();
        }
        else if (_wcsicmp(name, L"jump") == 0 && state.state)
        {
            return std::make_shared<JumpingState>();
        }

        return nullptr;
    }

    /* Crouch
        Triggered from idle or landing
        If not crouching on update (after jumping or released key), then walks/stands idle
    */
    StatePtr CrouchingState::onUpdate(PlayerNewtonBody *player, float frameTime)
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

    StatePtr CrouchingState::onAction(PlayerNewtonBody *player, const wchar_t *name, const Plugin::ActionState &state)
    {
        return nullptr;
    }

    /* Walk
        Move when keys pressed
        Jump when jumping
        Idle when no key pressed
    */
    StatePtr WalkingState::onUpdate(PlayerNewtonBody *player, float frameTime)
    {
        if (!player->moveForward && !player->moveBackward && !player->strafeLeft && !player->strafeRight)
        {
            return std::make_shared<IdleState>();
        }

        player->forwardSpeed += (((player->moveForward ? 1.0f : 0.0f) + (player->moveBackward ? -1.0f : 0.0f)) * 5.0f);
        player->lateralSpeed += (((player->strafeLeft ? -1.0f : 0.0f) + (player->strafeRight ? 1.0f : 0.0f)) * 5.0f);
        return nullptr;
    }

    StatePtr WalkingState::onAction(PlayerNewtonBody *player, const wchar_t *name, const Plugin::ActionState &state)
    {
        if (_wcsicmp(name, L"jump") == 0 && state.state)
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
    void JumpingState::onEnter(PlayerNewtonBody *player)
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

    void JumpingState::onExit(PlayerNewtonBody *player)
    {
        player->jumping = false;
    }

    StatePtr JumpingState::onUpdate(PlayerNewtonBody *player, float frameTime)
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
    void FallingState::onEnter(PlayerNewtonBody *player)
    {
        time = 0.0f;
    }

    StatePtr FallingState::onUpdate(PlayerNewtonBody *player, float frameTime)
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
    StatePtr DroppingState::onUpdate(PlayerNewtonBody *player, float frameTime)
    {
        if (player->touchingSurface)
        {
            return std::make_shared<IdleState>();
        }

        return nullptr;
    }

    namespace Components
    {
        PlayerBody::PlayerBody(void)
        {
        }

        void PlayerBody::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"height", height);
            saveParameter(componentData, L"outer_radius", outerRadius);
            saveParameter(componentData, L"inner_radius", innerRadius);
            saveParameter(componentData, L"stair_step", stairStep);
        }

        void PlayerBody::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            height = loadParameter(componentData, L"height", 6.0f);
            outerRadius = loadParameter(componentData, L"outer_radius", 1.5f);
            innerRadius = loadParameter(componentData, L"inner_radius", 0.5f);
            stairStep = loadParameter(componentData, L"stair_step", 1.5f);
        }
    }; // namespace Components

    GEK_CONTEXT_USER(PlayerBody)
        , public Plugin::ComponentMixin<Components::PlayerBody>
    {
    public:
        PlayerBody(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"player_body";
        }
    };

    GEK_REGISTER_CONTEXT_USER(PlayerBody)
}; // namespace Gek