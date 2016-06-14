#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\NewtonEntity.h"
#include "GEK\Newton\PlayerBody.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Newton\PlayerBody.h"
#include "GEK\Newton\NewtonProcessor.h"
#include <Newton.h>
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

    GEK_INTERFACE(State)
    {
        virtual void onEnter(StatePtr previousState) { };
        virtual void onExit(StatePtr nextState) { };

        virtual StatePtr onUpdate(float frameTime) { return nullptr; };
        virtual StatePtr onAction(const wchar_t *name, const ActionParam &param) { return nullptr; };
    };

    class PlayerNewtonBody;
    State *createIdleState(PlayerNewtonBody *playerBody);
    State *createCrouchgingState(PlayerNewtonBody *playerBody);
    State *createWalkingState(PlayerNewtonBody *playerBody, const wchar_t *trigger);
    State *createJumpingState(PlayerNewtonBody *playerBody);
    State *createFallingState(PlayerNewtonBody *playerBody);

    class PlayerNewtonBody
        : public EngineObserver
        , public NewtonEntity
    {
    public:
        LPVOID operator new(size_t size)
        {
            return _mm_malloc(size * sizeof(PlayerNewtonBody), 16);
        }

        void operator delete(LPVOID data)
        {
            _mm_free(data);
        }

    private:
        EngineContext *engine;
        NewtonProcessor *newtonProcessor;
        NewtonWorld *newtonWorld;

        Entity *entity;
        float halfHeight;
        float maximumSlope;
        float restrainingDistance;
        float sphereCastOrigin;

        NewtonBody *newtonBody;
        NewtonCollision *newtonCastingShape;
        NewtonCollision *newtonSupportShape;
        NewtonCollision *newtonUpperBodyShape;

        StatePtr currentState;
        float headingAngle;
        float forwardSpeed;
        float lateralSpeed;
        float verticalSpeed;

        Math::Float3 groundNormal;
        Math::Float3 groundVelocity;

        bool isJumpingState;

    public:
        PlayerNewtonBody(EngineContext *engine, 
            NewtonWorld *newtonWorld,
            Entity *entity)
            : engine(engine)
            , newtonProcessor(static_cast<NewtonProcessor *>(NewtonWorldGetUserData(newtonWorld)))
            , newtonWorld(newtonWorld)
            , newtonBody(nullptr)
            , entity(entity)
            , halfHeight(0.0f)
            , currentState(createIdleState(this))
            , headingAngle(0.0f)
            , forwardSpeed(0.0f)
            , lateralSpeed(0.0f)
            , verticalSpeed(0.0f)
            , groundNormal(0.0f)
            , groundVelocity(0.0f)
            , isJumpingState(false)
        {
            auto &mass = entity->getComponent<MassComponent>();
            auto &transform = entity->getComponent<TransformComponent>();
            auto &player = entity->getComponent<PlayerBodyComponent>();

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
            NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.5f, 0, outerShapeMatrix.data);
            NewtonCollisionSetScale(bodyCapsule, capsuleHeight, (player.outerRadius * 4.0f), (player.outerRadius * 4.0f));

            // compound collision player controller
            NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
            NewtonCompoundCollisionBeginAddRemove(playerShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
            NewtonCompoundCollisionEndAddRemove(playerShape);

            // create the kinematic body
            newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, Math::Float4x4::Identity.data);

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
                rotation.setPitchRotation(currentPoint * 2.0f * 3.141592f / numberOfSteps);
                convexPoints[0][currentPoint] = (rotation * point0);
                convexPoints[1][currentPoint] = (rotation * point1);
            }

            newtonCastingShape = NewtonCreateConvexHull(newtonWorld, (numberOfSteps * 2), convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, nullptr);
            newtonSupportShape = NewtonCompoundCollisionGetCollisionFromNode(bodyShape, NewtonCompoundCollisionGetNodeByIndex(bodyShape, 0));
            newtonUpperBodyShape = NewtonCompoundCollisionGetCollisionFromNode(bodyShape, NewtonCompoundCollisionGetNodeByIndex(bodyShape, 1));

            NewtonDestroyCollision(bodyCapsule);
            NewtonDestroyCollision(supportShape);
            NewtonDestroyCollision(playerShape);

            Math::Float4x4 matrix(transform.getMatrix());
            matrix.translation -= (matrix.ny * player.height);
            NewtonBodySetMatrix(newtonBody, matrix.data);
            NewtonBodySetUserData(newtonBody, dynamic_cast<NewtonEntity *>(this));
        }

        ~PlayerNewtonBody(void)
        {
            NewtonDestroyCollision(newtonCastingShape);
            NewtonDestroyCollision(newtonSupportShape);
            NewtonDestroyCollision(newtonUpperBodyShape);
            engine->removeObserver((EngineObserver *)this);
        }

        void addPlayerVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed)
        {
            this->forwardSpeed += forwardSpeed;
            this->lateralSpeed += lateralSpeed;
            this->verticalSpeed += verticalSpeed;
        }

        void setJumping(void)
        {
            isJumpingState = true;
        }

        bool isJumping(void)
        {
            return isJumpingState;
        }

        bool isFalling(void)
        {
            Math::Float3 velocity;
            NewtonBodyGetVelocity(newtonBody, velocity.data);
            return (velocity.y < 0.0f);
        }

        // EngineObserver
        void onAction(const wchar_t *name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"turn") == 0)
            {
                headingAngle += (param.value * 0.01f);
            }

            StatePtr nextState(currentState ? currentState->onAction(name, param) : nullptr);
            if (nextState)
            {
                currentState->onExit(nextState);
                nextState->onEnter(currentState);
                currentState = nextState;
            }
        }

        // NewtonEntity
        Entity * const getEntity(void) const
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
            StatePtr nextState(currentState ? currentState->onUpdate(frameTime) : nullptr);
            if (nextState)
            {
                currentState->onExit(nextState);
                nextState->onEnter(currentState);
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
            auto &transform = entity->getComponent<TransformComponent>();
            auto &player = entity->getComponent<PlayerBodyComponent>();

            // get the body motion state 
            Math::Float4x4 matrix;
            NewtonBodyGetMatrix(newtonBody, matrix.data);

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
            upConstraint.m_normal[0] = 0.0f;
            upConstraint.m_normal[1] = 1.0f;
            upConstraint.m_normal[2] = 0.0f;

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

            // determine if player is standing on some plane
            Math::Float4x4 supportMatrix(matrix);
            supportMatrix.translation += (matrix.ny * sphereCastOrigin);
            if (isJumping() || isFalling())
            {
                Math::Float3 destination(matrix.translation);
                updateGroundPlane(matrix, supportMatrix, destination, threadHandle);
            }
            else
            {
                velocityStep = std::abs(matrix.ny.dot(velocity * frameTime));
                float castDist = ((groundNormal.getLengthSquared() > 0.0f) ? player.stairStep : velocityStep);
                Math::Float3 destination(matrix.translation - (matrix.ny * (castDist * 2.0f)));
                updateGroundPlane(matrix, supportMatrix, destination, threadHandle);
            }

            NewtonBodySetVelocity(newtonBody, velocity.data);
            NewtonBodySetMatrix(newtonBody, matrix.data);
            transform.position = matrix.translation + matrix.ny * player.height;
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

        Math::Quaternion integrateOmega(const Math::Quaternion &rotation, const Math::Float3& omega, float frameTime) const
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

        void setDesiredVelocity(const Math::Float4x4 &matrix, float frameTime) const
        {
            Math::Float3 gravity(newtonProcessor->getGravity(matrix.translation));

            Math::Float3 desiredVelocity;
            NewtonBodyGetVelocity(newtonBody, desiredVelocity.data);
            if ((verticalSpeed <= 0.0f) && (groundNormal.getLengthSquared() > 0.0f))
            {
                // plane is supported by a ground plane, apply the player input desiredVelocity
                if (groundNormal.dot(matrix.ny) >= maximumSlope)
                {
                    // player is in a legal slope, he is in full control of his movement
                    desiredVelocity = ((matrix.ny * desiredVelocity.dot(matrix.ny)) + (gravity * frameTime) + (matrix.nz * forwardSpeed) + (matrix.nx * lateralSpeed) + (matrix.ny * verticalSpeed));
                    desiredVelocity += (groundVelocity - (matrix.ny * matrix.ny.dot(groundVelocity)));

                    float speedLimitMagnitudeSquared = ((forwardSpeed * forwardSpeed) + (lateralSpeed * lateralSpeed) + (verticalSpeed * verticalSpeed) + groundVelocity.getLengthSquared() + 0.1f);
                    float speedMagnitudeSquared = desiredVelocity.getLengthSquared();
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
                    desiredVelocity += (matrix.ny * verticalSpeed);
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
                contactVelocity.set(0.0f, 0.0f, 0.0f);
            }

            const float restitution = 0.0f;
            Math::Float3 normal(contactInfo->m_normal);
            float reboundVelocityMagnitude = -((velocity - contactVelocity).dot(normal) * (1.0f + restitution));
            return std::max(0.0f, reboundVelocityMagnitude);
        }

        void updateGroundPlane(Math::Float4x4& matrix, const Math::Float4x4& castMatrix, const Math::Float3& destination, int threadHandle)
        {
            groundNormal.set(0.0f, 0.0f, 0.0f);
            groundVelocity.set(0.0f, 0.0f, 0.0f);

            float parameter = 10.0f;
            NewtonWorldConvexCastReturnInfo info;
            ConvexCastPreFilter preFilterData(newtonBody);
            if (NewtonWorldConvexCast(newtonWorld, castMatrix.data, destination.data, newtonCastingShape, &parameter, &preFilterData, ConvexCastPreFilter::preFilter, &info, 1, threadHandle) > 0 && parameter <= 1.0f)
            {
                isJumpingState = false;
                groundNormal.set(info.m_normal[0], info.m_normal[1], info.m_normal[2]);
                Math::Float3 supportPoint(castMatrix.translation + ((destination - castMatrix.translation) * parameter));
                NewtonBodyGetPointVelocity(info.m_hitBody, supportPoint.data, groundVelocity.data);
                matrix.translation = supportPoint;
            }
        }
    };

    class PlayerStateMixin
        : public State
    {
    protected:
        PlayerNewtonBody *playerBody;

    public:
        PlayerStateMixin(PlayerNewtonBody *playerBody)
            : playerBody(playerBody)
        {
        }
    };

    class IdleState
        : public PlayerStateMixin
    {
    public:
        IdleState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        StatePtr onUpdate(float frameTime)
        {
            playerBody->addPlayerVelocity(0.0f, 0.0f, 0.0f);
            return nullptr;
        }

        StatePtr onAction(const wchar_t *name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"crouch") == 0 && param.state)
            {
                return StatePtr(createCrouchgingState(playerBody));
            }
            else if (_wcsicmp(name, L"move_forward") == 0 && param.state)
            {
                return StatePtr(createWalkingState(playerBody, name));
            }
            else if (_wcsicmp(name, L"move_backward") == 0 && param.state)
            {
                return StatePtr(createWalkingState(playerBody, name));
            }
            else if (_wcsicmp(name, L"strafe_left") == 0 && param.state)
            {
                return StatePtr(createWalkingState(playerBody, name));
            }
            else if (_wcsicmp(name, L"strafe_right") == 0 && param.state)
            {
                return StatePtr(createWalkingState(playerBody, name));
            }
            else if (_wcsicmp(name, L"jump") == 0 && param.state)
            {
                return StatePtr(createJumpingState(playerBody));
            }

            return nullptr;
        }
    };

    class CrouchingState
        : public PlayerStateMixin
    {
    public:
        CrouchingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        StatePtr onUpdate(float frameTime)
        {
            playerBody->addPlayerVelocity(0.0f, 0.0f, 0.0f);
            return nullptr;
        }

        StatePtr onAction(const wchar_t *name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"crouch") == 0 && !param.state)
            {
                return StatePtr(createIdleState(playerBody));
            }

            return nullptr;
        }
    };

    class WalkingState
        : public PlayerStateMixin
    {
    private:
        bool moveForward;
        bool moveBackward;
        bool strafeLeft;
        bool strafeRight;

    public:
        WalkingState(PlayerNewtonBody *playerBody, const wchar_t *trigger)
            : PlayerStateMixin(playerBody)
            , moveForward(false)
            , moveBackward(false)
            , strafeLeft(false)
            , strafeRight(false)
        {
            onAction(trigger, true);
        }

        // State
        StatePtr onUpdate(float frameTime)
        {
            GEK_REQUIRE(playerBody);

            float lateralSpeed = (((moveForward ? 1.0f : 0.0f) + (moveBackward ? -1.0f : 0.0f)) * 5.0f);
            float strafeSpeed = (((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f);
            playerBody->addPlayerVelocity(lateralSpeed, strafeSpeed, 0.0f);

            return nullptr;
        }

        StatePtr onAction(const wchar_t *name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"move_forward") == 0)
            {
                moveForward = param.state;
            }
            else if (_wcsicmp(name, L"move_backward") == 0)
            {
                moveBackward = param.state;
            }
            else if (_wcsicmp(name, L"strafe_left") == 0)
            {
                strafeLeft = param.state;
            }
            else if (_wcsicmp(name, L"strafe_right") == 0)
            {
                strafeRight = param.state;
            }
            else if (_wcsicmp(name, L"jump") == 0 && param.state)
            {
                return StatePtr(createJumpingState(playerBody));
            }

            if (!moveForward &&
                !moveBackward &&
                !strafeLeft &&
                !strafeRight)
            {
                return StatePtr(createIdleState(playerBody));
            }

            return nullptr;
        }
    };

    class JumpingState
        : public PlayerStateMixin
    {
    private:
        float jumpVelocity;

    public:
        JumpingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
            , jumpVelocity(10.0f)
        {
            GEK_REQUIRE(playerBody);
            playerBody->setJumping();
        }

        // State
        StatePtr onUpdate(float frameTime)
        {
            GEK_REQUIRE(playerBody);

            playerBody->addPlayerVelocity(0.0f, 0.0f, jumpVelocity);
            jumpVelocity = 0.0f;

            if (!playerBody->isJumping())
            {
                return StatePtr(createIdleState(playerBody));
            }
            else if (playerBody->isFalling())
            {
                return StatePtr(createFallingState(playerBody));
            }

            return nullptr;
        }
    };

    class FallingState
        : public PlayerStateMixin
    {
    private:

    public:
        FallingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
            GEK_REQUIRE(playerBody);
        }

        // State
        StatePtr onUpdate(float frameTime)
        {
            GEK_REQUIRE(playerBody);

            playerBody->addPlayerVelocity(0.0f, 0.0f, 0.0f);

            if (!playerBody->isFalling())
            {
                return StatePtr(createIdleState(playerBody));
            }

            return nullptr;
        }
    };

    State *createIdleState(PlayerNewtonBody *playerBody)
    {
        return new IdleState(playerBody);
    }

    State *createCrouchgingState(PlayerNewtonBody *playerBody)
    {
        return new IdleState(playerBody);
    }

    State *createWalkingState(PlayerNewtonBody *playerBody, const wchar_t *trigger)
    {
        return new WalkingState(playerBody, trigger);
    }

    State *createJumpingState(PlayerNewtonBody *playerBody)
    {
        return new JumpingState(playerBody);
    }

    State *createFallingState(PlayerNewtonBody *playerBody)
    {
        return new FallingState(playerBody);
    }

    NewtonEntityPtr createPlayerBody(EngineContext *engine, NewtonWorld *newtonWorld, Entity *entity)
    {
        std::shared_ptr<PlayerNewtonBody> player;
        try
        {
            player = std::make_shared<PlayerNewtonBody>(engine, newtonWorld, entity);
        }
        catch (const std::bad_alloc &badAllocation)
        {
            GEK_THROW_EXCEPTION(Trace::Exception, "Unable to allocate new player object: %v", badAllocation.what());
        };

        engine->addObserver((EngineObserver *)player.get());
        return player;
    }

    PlayerBodyComponent::PlayerBodyComponent(void)
    {
    }

    void PlayerBodyComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"height", height);
        saveParameter(componentData, L"outer_radius", outerRadius);
        saveParameter(componentData, L"inner_radius", innerRadius);
        saveParameter(componentData, L"stair_step", stairStep);
    }

    void PlayerBodyComponent::load(const Population::ComponentDefinition &componentData)
    {
        height = loadParameter(componentData, L"height", 6.0f);
        outerRadius = loadParameter(componentData, L"outer_radius", 1.5f);
        innerRadius = loadParameter(componentData, L"inner_radius", 0.5f);
        stairStep = loadParameter(componentData, L"stair_step", 1.5f);
    }

    class PlayerBodyImplementation
        : public ContextRegistration<PlayerBodyImplementation>
        , public ComponentMixin<PlayerBodyComponent>
    {
    public:
        PlayerBodyImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"player_body";
        }
    };

    GEK_REGISTER_CONTEXT_USER(PlayerBodyImplementation)
}; // namespace Gek