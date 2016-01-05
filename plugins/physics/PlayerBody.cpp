#include "GEK\Newton\PlayerBody.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Engine\Action.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\NewtonEntity.h"
#include "GEK\Newton\PlayerBody.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Matrix4x4.h"
#include <Newton.h>

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
    static const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);

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

    class ConvexRayFilter : public ConvexCastPreFilter
    {
    public:
        Math::Float3 hitPoint;
        Math::Float3 hitNormal;
        const NewtonBody* hitBody;
        const NewtonCollision* hitShape;
        long long collisionIdentifier;
        float intersectionDistance;

    public:
        ConvexRayFilter(const NewtonBody* const sourceBody)
            : ConvexCastPreFilter(sourceBody)
            , hitBody(NULL)
            , hitShape(NULL)
            , collisionIdentifier(0)
            , intersectionDistance(1.2f)
        {
        }

        static float filter(const NewtonBody* const hitBody, const NewtonCollision* const hitShape, const float* const hitContact, const float* const hitNormal, long long collisionIdentifier, void* const userData, float intersectParam)
        {
            ConvexRayFilter* const filterData = static_cast<ConvexRayFilter * const>(userData);
            if (intersectParam < filterData->intersectionDistance)
            {
                filterData->hitBody = hitBody;
                filterData->hitShape = hitShape;
                filterData->collisionIdentifier = collisionIdentifier;
                filterData->intersectionDistance = intersectParam;
                filterData->hitPoint = *(Math::Float3 *)hitContact;
                filterData->hitNormal = *(Math::Float3 *)hitNormal;
            }

            return intersectParam;
        }
    };

    DECLARE_INTERFACE(State)
    {
        STDMETHOD_(void, onEnter)       (THIS) { };
        STDMETHOD_(void, onExit)        (THIS) { };

        STDMETHOD_(State *, onUpdate)   (THIS_ float frameTime) { return nullptr; };
        STDMETHOD_(State *, onAction)   (THIS_ LPCWSTR name, const ActionParam &param) { return nullptr; };
    };

    class PlayerNewtonBody;
    State *createIdleState(PlayerNewtonBody *playerBody);
    State *createCrouchgingState(PlayerNewtonBody *playerBody);
    State *createWalkingState(PlayerNewtonBody *playerBody, LPCWSTR trigger);
    State *createJumpingState(PlayerNewtonBody *playerBody);
    State *createFallingState(PlayerNewtonBody *playerBody);

    class PlayerNewtonBody : public UnknownMixin
        , virtual public ActionObserver
        , virtual public NewtonEntity
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
        IUnknown *actionProvider;

        Entity *entity;
        PlayerBodyComponent &playerBodyComponent;
        TransformComponent &transformComponent;
        MassComponent &massComponent;
        Math::Float4x4 movementBasis;
        float maximumSlope;
        float restrainingDistance;
        float sphereCastOrigin;

        NewtonWorld *newtonWorld;
        NewtonBody *newtonBody;
        NewtonCollision *newtonCastingShape;
        NewtonCollision *newtonSupportShape;
        NewtonCollision *newtonUpperBodyShape;

        State *currentState;
        float headingAngle;
        float forwardSpeed;
        float lateralSpeed;
        float verticalSpeed;

        Math::Float3 groundNormal;
        Math::Float3 groundVelocity;

        const NewtonBody *onNewtonBody;

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
            // this is correct
            float omegaMagnitudeSquared = omega.dot(omega);
            const float errorAngle = 0.0125f * 3.141592f / 180.0f;
            const float errorAngleSquared = errorAngle * errorAngle;
            if (omegaMagnitudeSquared > errorAngleSquared)
            {
                float inverseOmegaMagnitude = 1.0f / std::sqrt(omegaMagnitudeSquared);
                Math::Float3 omegaAxis(omega * (inverseOmegaMagnitude));
                float omegaAngle = (inverseOmegaMagnitude * omegaMagnitudeSquared * frameTime);

                Math::Quaternion deltaRotation(omegaAxis, omegaAngle);
                return ((rotation * deltaRotation) * (1.0f / std::sqrt(rotation.dot(rotation))));
            }
            else
            {
                return rotation;
            }
        }

        Math::Float3 calculateAverageOmega(const Math::Quaternion &_quaternion0, const Math::Quaternion &quaternion1, float inverseFrameTime) const
        {
            Math::Quaternion quaternion0(_quaternion0);
            if (quaternion0.dot(quaternion1) < 0.0f)
            {
                quaternion0 *= -1.0f;
            }

            Math::Quaternion deltaQuaternion(quaternion0.getInverse() * quaternion1);
            Math::Float3 omegaDirection(deltaQuaternion.x, deltaQuaternion.y, deltaQuaternion.z);

            float directionMagnitudeSquared = omegaDirection.dot(omegaDirection);
            if (directionMagnitudeSquared < PLAYER_EPSILON_SQUARED)
            {
                return 0.0f;
            }

            float inverseDirectionMagnitude = (1.0f / std::sqrt(directionMagnitudeSquared));
            float directionMagnitude = (directionMagnitudeSquared * inverseDirectionMagnitude);

            float omegaMagnitude = (2.0f * std::atan2(directionMagnitude, deltaQuaternion.w) * inverseFrameTime);
            return (omegaDirection * (inverseDirectionMagnitude * omegaMagnitude));
        }

        Math::Float3 calculateDesiredOmega(float frameTime) const
        {
            float playerRotationData[4];
            NewtonBodyGetRotation(newtonBody, playerRotationData);
            Math::Quaternion targetRotation(movementBasis.ny, headingAngle);
            return calculateAverageOmega({ playerRotationData[1], playerRotationData[2], playerRotationData[3], playerRotationData[0] }, targetRotation, (0.5f / frameTime));
        }

        Math::Float3 calculateDesiredVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed, float frameTime) const
        {
            Math::Float4x4 matrix;
            NewtonBodyGetMatrix(newtonBody, matrix.data);
            Math::Float4x4 localBasis(matrix * movementBasis);

            Math::Float3 desiredVelocity;
            if ((verticalSpeed <= 0.0f) && (groundNormal.dot(groundNormal) > 0.0f))
            {
                // plane is supported by a ground plane, apply the player input desiredVelocity
                if (groundNormal.dot(localBasis.ny) >= maximumSlope)
                {
                    // player is in a legal slope, he is in full control of his movement
                    Math::Float3 bodyVelocity;
                    NewtonBodyGetVelocity(newtonBody, bodyVelocity.data);
                    desiredVelocity = ((localBasis.ny * bodyVelocity.dot(localBasis.ny)) + (Gravity * frameTime) + (localBasis.nz * forwardSpeed) + (localBasis.nx * lateralSpeed) + (localBasis.ny * verticalSpeed));
                    desiredVelocity += (groundVelocity - (localBasis.ny * localBasis.ny.dot(groundVelocity)));

                    float speedLimitMagnitudeSquared = ((forwardSpeed * forwardSpeed) + (lateralSpeed * lateralSpeed) + (verticalSpeed * verticalSpeed) + groundVelocity.dot(groundVelocity) + 0.1f);
                    float speedMagnitudeSquared = desiredVelocity.dot(desiredVelocity);
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
                    NewtonBodyGetVelocity(newtonBody, desiredVelocity.data);
                    desiredVelocity += (localBasis.ny * verticalSpeed);
                    desiredVelocity += (Gravity * frameTime);
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
                NewtonBodyGetVelocity(newtonBody, desiredVelocity.data);
                desiredVelocity += (localBasis.ny * verticalSpeed);
                desiredVelocity += (Gravity * frameTime);
            }

            return desiredVelocity;
        }

        float calculateContactKinematics(const Math::Float3& velocity, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
        {
            Math::Float3 contactVelocity;
            if (contactInfo->m_hitBody)
            {
                NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, contactVelocity.data);
            }

            const float restitution = 0.0f;
            Math::Float3 normal(contactInfo->m_normal);
            float reboundVelocityMagnitude = -((velocity - contactVelocity).dot(normal) * (1.0f + restitution));
            return std::max(0.0f, reboundVelocityMagnitude);
        }

        void updateGroundPlane(Math::Float4x4& matrix, const Math::Float4x4& castMatrix, const Math::Float3& destination, int threadHandle)
        {
            ConvexRayFilter filterData(newtonBody);
            NewtonWorldConvexRayCast(newtonWorld, newtonCastingShape, castMatrix.data, destination.data, ConvexRayFilter::filter, &filterData, ConvexCastPreFilter::preFilter, threadHandle);

            groundNormal.set(0.0f);
            groundVelocity.set(0.0f);
            if (onNewtonBody = filterData.hitBody)
            {
                groundNormal = filterData.hitNormal;
                Math::Float3 supportPoint(castMatrix.translation + ((destination - castMatrix.translation) * filterData.intersectionDistance));
                NewtonBodyGetPointVelocity(filterData.hitBody, supportPoint.data, groundVelocity.data);
                matrix.translation = supportPoint;
            }
        }

    public:
        void addPlayerVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed)
        {
            this->forwardSpeed += forwardSpeed;
            this->lateralSpeed += lateralSpeed;
            this->verticalSpeed += verticalSpeed;
        }

        bool isOnSomething(void)
        {
            return (onNewtonBody ? true : false);
        }

    public:
        PlayerNewtonBody(IUnknown *actionProvider, NewtonWorld *newtonWorld, Entity *entity,
            PlayerBodyComponent &playerBodyComponent,
            TransformComponent &transformComponent,
            MassComponent &massComponent)
            : actionProvider(actionProvider)
            , newtonWorld(newtonWorld)
            , newtonBody(nullptr)
            , entity(entity)
            , playerBodyComponent(playerBodyComponent)
            , transformComponent(transformComponent)
            , massComponent(massComponent)
            , currentState(createIdleState(this))
            , headingAngle(0.0f)
            , forwardSpeed(0.0f)
            , lateralSpeed(0.0f)
            , verticalSpeed(0.0f)
            , onNewtonBody(nullptr)
        {
            movementBasis.nx.set( 1.0f, 0.0f, 0.0f);
            movementBasis.ny.set( 0.0f, 1.0f, 0.0f);
            movementBasis.nz.set( 0.0f, 0.0f, 1.0f);

            setRestrainingDistance(0.01f);

            setClimbSlope(45.0f * 3.1416f / 180.0f);

            const int numberOfSteps = 12;
            Math::Float3 convexPoints[2][numberOfSteps];

            // create an inner thin cylinder
            Math::Float3 point0(0.0f, playerBodyComponent.innerRadius, 0.0f);
            Math::Float3 point1(playerBodyComponent.height, playerBodyComponent.innerRadius, 0.0f);
            for (int index = 0; index < numberOfSteps; index++)
            {
                Math::Float4x4 rotation;
                rotation.setPitchRotation(index * 2.0f * 3.141592f / numberOfSteps);
                convexPoints[0][index] = (movementBasis * rotation * point0);
                convexPoints[1][index] = (movementBasis * rotation * point1);
            }

            NewtonCollision* const supportShape = NewtonCreateConvexHull(newtonWorld, (numberOfSteps * 2), convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, NULL);

            // create the outer thick cylinder
            Math::Float4x4 outerShapeMatrix(movementBasis);
            float capsuleHeight = (playerBodyComponent.height - playerBodyComponent.stairStep);
            sphereCastOrigin = ((capsuleHeight * 0.5f) + playerBodyComponent.stairStep);
            outerShapeMatrix.translation = (outerShapeMatrix.nx * sphereCastOrigin);
            NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.5f, 0, outerShapeMatrix.data);
            NewtonCollisionSetScale(bodyCapsule, capsuleHeight, (playerBodyComponent.outerRadius * 4.0f), (playerBodyComponent.outerRadius * 4.0f));

            // compound collision player controller
            NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
            NewtonCompoundCollisionBeginAddRemove(playerShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
            NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
            NewtonCompoundCollisionEndAddRemove(playerShape);

            // create the kinematic body
            newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, Math::Float4x4().data);

            // players must have weight, otherwise they are infinitely strong when they collide
            NewtonCollision* const bodyShape = NewtonBodyGetCollision(newtonBody);
            NewtonBodySetMassProperties(newtonBody, massComponent, bodyShape);

            // make the body collidable with other dynamics bodies, by default
            NewtonBodySetCollidable(newtonBody, true);

            float castHeight = (capsuleHeight * 0.4f);
            float castRadius = std::max(0.5f, (playerBodyComponent.innerRadius * 0.5f));

            point0.set(0.0f, castRadius, 0.0f);
            point1.set(castHeight, castRadius, 0.0f);
            for (int index = 0; index < numberOfSteps; index++)
            {
                Math::Float4x4 rotation;
                rotation.setPitchRotation(index * 2.0f * 3.141592f / numberOfSteps);
                convexPoints[0][index] = (movementBasis * rotation * point0);
                convexPoints[1][index] = (movementBasis * rotation * point1);
            }

            newtonCastingShape = NewtonCreateConvexHull(newtonWorld, (numberOfSteps * 2), convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, NULL);
            newtonSupportShape = NewtonCompoundCollisionGetCollisionFromNode(bodyShape, NewtonCompoundCollisionGetNodeByIndex(bodyShape, 0));
            newtonUpperBodyShape = NewtonCompoundCollisionGetCollisionFromNode(bodyShape, NewtonCompoundCollisionGetNodeByIndex(bodyShape, 1));

            NewtonDestroyCollision(bodyCapsule);
            NewtonDestroyCollision(supportShape);
            NewtonDestroyCollision(playerShape);

            NewtonBodySetMatrix(newtonBody, transformComponent.getMatrix().data);
            NewtonBodySetUserData(newtonBody, dynamic_cast<NewtonEntity *>(this));
        }

        ~PlayerNewtonBody(void)
        {
            NewtonDestroyCollision(newtonCastingShape);
            NewtonDestroyCollision(newtonSupportShape);
            NewtonDestroyCollision(newtonUpperBodyShape);
            ObservableMixin::removeObserver(actionProvider, getClass<ActionObserver>());
        }

        // IUnknown
        BEGIN_INTERFACE_LIST(PlayerNewtonBody)
            INTERFACE_LIST_ENTRY_COM(ActionObserver);
        END_INTERFACE_LIST_UNKNOWN

        // ActionObserver
        STDMETHODIMP_(void) onAction(LPCWSTR name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"turn") == 0)
            {
                headingAngle += (param.value * 0.01f);
            }

            State *newState = (currentState ? currentState->onAction(name, param) : nullptr);
            if (newState)
            {
                currentState->onExit();
                delete currentState;
                currentState = newState;
                newState->onEnter();
            }
        }

        // NewtonEntity
        STDMETHODIMP_(Entity *) getEntity(void) const
        {
            return entity;
        }

        STDMETHODIMP_(NewtonBody *) getNewtonBody(void) const
        {
            return newtonBody;
        }

        STDMETHODIMP_(void) onPreUpdate(float frameTime, int threadHandle)
        {
            forwardSpeed = 0.0f;
            lateralSpeed = 0.0f;
            verticalSpeed = 0.0f;
            State *newState = (currentState ? currentState->onUpdate(frameTime) : nullptr);
            if (newState)
            {
                currentState->onExit();
                delete currentState;
                currentState = newState;
                newState->onEnter();
            }

            Math::Float3 omega(calculateDesiredOmega(frameTime));
            NewtonBodySetOmega(newtonBody, omega.data);

            Math::Float3 velocity(calculateDesiredVelocity(forwardSpeed, lateralSpeed, verticalSpeed, frameTime));
            NewtonBodySetVelocity(newtonBody, velocity.data);
        }

        STDMETHODIMP_(void) onPostUpdate(float frameTime, int threadHandle)
        {
            // get the body motion state 
            Math::Float4x4 matrix;
            NewtonBodyGetMatrix(newtonBody, matrix.data);
            Math::Float4x4 localBasis(matrix * movementBasis);

            Math::Float3 omega;
            NewtonBodyGetOmega(newtonBody, omega.data);

            // integrate body angular velocity
            Math::Quaternion bodyRotation(integrateOmega(matrix.getQuaternion(), omega, frameTime));
            matrix = bodyRotation.getMatrix(matrix.translation);

            // integrate linear velocity
            Math::Float3 velocity;
            NewtonBodyGetVelocity(newtonBody, velocity.data);
            float normalizedTimeLeft = 1.0f;
            float step = (frameTime * velocity.getLength());
            float descreteframeTime = (frameTime * (1.0f / D_PLAYER_DESCRETE_MOTION_STEPS));
            int previousContactCount = 0;
            ConvexCastPreFilter preFilterData(newtonBody);
            NewtonWorldConvexCastReturnInfo previousInfo[D_PLAYER_CONTROLLER_MAX_CONTACTS];

            Math::Float3 scale;
            NewtonCollisionGetScale(newtonUpperBodyShape, &scale.x, &scale.y, &scale.z);
            //const float radius = (playerBodyComponent.outerRadius * 4.0f);
            const float radius = ((playerBodyComponent.outerRadius + restrainingDistance) * 4.0f);
            NewtonCollisionSetScale(newtonUpperBodyShape, playerBodyComponent.height - playerBodyComponent.stairStep, radius, radius);

            NewtonWorldConvexCastReturnInfo upConstraint;
            memset(&upConstraint, 0, sizeof(upConstraint));
            upConstraint.m_normal[0] = movementBasis.ny.x;
            upConstraint.m_normal[1] = movementBasis.ny.y;
            upConstraint.m_normal[2] = movementBasis.ny.z;

            for (int step = 0; ((step < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedTimeLeft > PLAYER_EPSILON)); step++)
            {
                if (velocity.dot(velocity) < PLAYER_EPSILON)
                {
                    break;
                }

                float timeToImpact = 0.0f;
                NewtonWorldConvexCastReturnInfo info[D_PLAYER_CONTROLLER_MAX_CONTACTS];
                Math::Float3 destinationPosition(matrix.translation + (velocity * frameTime));
                int contactCount = NewtonWorldConvexCast(newtonWorld, matrix.data, destinationPosition.data, newtonUpperBodyShape, &timeToImpact, &preFilterData, ConvexCastPreFilter::preFilter, info, ARRAYSIZE(info), threadHandle);
                if (contactCount)
                {
                    matrix.translation += (velocity * (timeToImpact * frameTime));
                    if (timeToImpact > 0.0f)
                    {
                        matrix.translation -= (velocity * (D_PLAYER_CONTACT_SKIN_THICKNESS / velocity.getLength()));
                    }

                    normalizedTimeLeft -= timeToImpact;

                    float speed[D_PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                    float bounceSpeed[D_PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                    Math::Float3 bounceNormal[D_PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                    for (int index = 1; index < contactCount; index++)
                    {
                        Math::Float3 normal0(info[index - 1].m_normal);
                        for (int contact = 0; contact < index; contact++)
                        {
                            Math::Float3 normal1(info[contact].m_normal);
                            if (normal0.dot(normal1) > 0.9999f)
                            {
                                info[index] = info[contactCount - 1];
                                index--;
                                contactCount--;
                                break;
                            }
                        }
                    }

                    int bounceCount = 0;
                    if (verticalSpeed == 0.0f)
                    {
                        upConstraint.m_point[0] = matrix.translation.x;
                        upConstraint.m_point[1] = matrix.translation.y;
                        upConstraint.m_point[2] = matrix.translation.z;

                        speed[bounceCount] = 0.0f;
                        bounceNormal[bounceCount].set(upConstraint.m_normal);
                        bounceSpeed[bounceCount] = calculateContactKinematics(velocity, &upConstraint);
                        bounceCount++;
                    }

                    for (int index = 0; index < contactCount; index++)
                    {
                        speed[bounceCount] = 0.0f;
                        bounceNormal[bounceCount].set(info[index].m_normal);
                        bounceSpeed[bounceCount] = calculateContactKinematics(velocity, &info[index]);
                        bounceCount++;
                    }

                    for (int index = 0; index < previousContactCount; index++)
                    {
                        speed[bounceCount] = 0.0f;
                        bounceNormal[bounceCount] = Math::Float3(previousInfo[index].m_normal);
                        bounceSpeed[bounceCount] = calculateContactKinematics(velocity, &previousInfo[index]);
                        bounceCount++;
                    }

                    float residual = 10.0f;
                    Math::Float3 auxiliaryBounceVelocity;
                    for (int index = 0; ((index < D_PLAYER_MAX_SOLVER_ITERATIONS) && (residual > PLAYER_EPSILON)); index++)
                    {
                        residual = 0.0f;
                        for (int bounce = 0; bounce < bounceCount; bounce++)
                        {
                            Math::Float3 normal(bounceNormal[bounce]);
                            float value = (bounceSpeed[bounce] - normal.dot(auxiliaryBounceVelocity));
                            float delta = (speed[bounce] + value);
                            if (delta < 0.0f)
                            {
                                value = 0.0f;
                                delta = 0.0f;
                            }

                            residual = std::max(std::abs(value), residual);
                            auxiliaryBounceVelocity += (normal * (delta - speed[bounce]));
                            speed[bounce] = delta;
                        }
                    }

                    Math::Float3 velocityStep;
                    for (int index = 0; index < bounceCount; index++)
                    {
                        Math::Float3 normal(bounceNormal[index]);
                        velocityStep += (normal * speed[index]);
                    }

                    velocity += velocityStep;
                    float velocityMagnitudeSquared = velocityStep.dot(velocityStep);
                    if (velocityMagnitudeSquared < PLAYER_EPSILON_SQUARED)
                    {
                        float advanceTime = std::min(descreteframeTime, (normalizedTimeLeft * frameTime));
                        matrix.translation += (velocity * advanceTime);
                        normalizedTimeLeft -= (advanceTime / frameTime);
                    }

                    previousContactCount = contactCount;
                    memcpy(previousInfo, info, (previousContactCount * sizeof(NewtonWorldConvexCastReturnInfo)));
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
            supportMatrix.translation += (localBasis.ny * sphereCastOrigin);
            if (verticalSpeed > 0.0f)
            {
                Math::Float3 destination(matrix.translation);
                updateGroundPlane(matrix, supportMatrix, destination, threadHandle);
            }
            else
            {
                step = std::abs(localBasis.ny.dot(velocity * frameTime));
                float castDist = ((groundNormal.dot(groundNormal) > 0.0f) ? playerBodyComponent.stairStep : step);
                Math::Float3 destination(matrix.translation - (localBasis.ny * (castDist * 2.0f)));
                updateGroundPlane(matrix, supportMatrix, destination, threadHandle);
            }

            // set player velocity, position and orientation
            NewtonBodySetVelocity(newtonBody, velocity.data);
            NewtonBodySetMatrix(newtonBody, matrix.data);

            transformComponent.position = matrix.translation;
            transformComponent.rotation = matrix.getQuaternion();
        }
    };

    class PlayerStateMixin : public State
    {
    protected:
        PlayerNewtonBody *playerBody;

    public:
        PlayerStateMixin(PlayerNewtonBody *playerBody)
            : playerBody(playerBody)
        {
        }
    };

    class IdleState : public PlayerStateMixin
    {
    public:
        IdleState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            playerBody->addPlayerVelocity(0.0f, 0.0f, 0.0f);
            return nullptr;
        }

        STDMETHODIMP_(State *) onAction(LPCWSTR name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"crouch") == 0 && param.state)
            {
                return createCrouchgingState(playerBody);
            }
            else if (_wcsicmp(name, L"move_forward") == 0 && param.state)
            {
                return createWalkingState(playerBody, name);
            }
            else if (_wcsicmp(name, L"move_backward") == 0 && param.state)
            {
                return createWalkingState(playerBody, name);
            }
            else if (_wcsicmp(name, L"strafe_left") == 0 && param.state)
            {
                return createWalkingState(playerBody, name);
            }
            else if (_wcsicmp(name, L"strafe_right") == 0 && param.state)
            {
                return createWalkingState(playerBody, name);
            }
            else if (_wcsicmp(name, L"jump") == 0 && param.state)
            {
                return createJumpingState(playerBody);
            }

            return nullptr;
        }
    };

    class CrouchingState : public PlayerStateMixin
    {
    public:
        CrouchingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            playerBody->addPlayerVelocity(0.0f, 0.0f, 0.0f);
            return nullptr;
        }

        STDMETHODIMP_(State *) onAction(LPCWSTR name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"crouch") == 0 && !param.state)
            {
                return createIdleState(playerBody);
            }

            return nullptr;
        }
    };

    class WalkingState : public PlayerStateMixin
    {
    private:
        bool moveForward;
        bool moveBackward;
        bool strafeLeft;
        bool strafeRight;

    public:
        WalkingState(PlayerNewtonBody *playerBody, LPCWSTR trigger)
            : PlayerStateMixin(playerBody)
            , moveForward(false)
            , moveBackward(false)
            , strafeLeft(false)
            , strafeRight(false)
        {
            onAction(trigger, true);
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            REQUIRE_RETURN(playerBody, nullptr);

            float lateralSpeed = (((moveForward ? 1.0f : 0.0f) + (moveBackward ? -1.0f : 0.0f)) * 5.0f);
            float strafeSpeed = (((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f);
            playerBody->addPlayerVelocity(lateralSpeed, strafeSpeed, 0.0f);

            return nullptr;
        }

        STDMETHODIMP_(State *) onAction(LPCWSTR name, const ActionParam &param)
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
                return createJumpingState(playerBody);
            }

            if (!moveForward &&
                !moveBackward &&
                !strafeLeft &&
                !strafeRight)
            {
                return createIdleState(playerBody);
            }

            return nullptr;
        }
    };

    class JumpingState : public PlayerStateMixin
    {
    private:
        float jumpVelocity;

    public:
        JumpingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
            , jumpVelocity(10.0f)
        {
            REQUIRE_VOID_RETURN(playerBody);
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            REQUIRE_RETURN(playerBody, nullptr);

            playerBody->addPlayerVelocity(0.0f, 0.0f, jumpVelocity);
            jumpVelocity = 0.0f;

            if (playerBody->isOnSomething())
            {
                return createIdleState(playerBody);
            }

            return nullptr;
        }
    };

    class FallingState : public PlayerStateMixin
    {
    public:
        FallingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            REQUIRE_RETURN(playerBody, nullptr);

            playerBody->addPlayerVelocity(0.0f, 0.0f, 0.0f);

            if (playerBody->isOnSomething())
            {
                return createIdleState(playerBody);
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

    State *createWalkingState(PlayerNewtonBody *playerBody, LPCWSTR trigger)
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

    NewtonEntity *createPlayerBody(IUnknown *actionProvider, NewtonWorld *newtonWorld, Entity *entity, PlayerBodyComponent &playerBodyComponent, TransformComponent &transformComponent, MassComponent &massComponent)
    {
        PlayerNewtonBody *playerBody = new PlayerNewtonBody(actionProvider, newtonWorld, entity, playerBodyComponent, transformComponent, massComponent);
        ObservableMixin::addObserver(actionProvider, playerBody->getClass<ActionObserver>());
        return playerBody;
    }

    PlayerBodyComponent::PlayerBodyComponent(void)
        : outerRadius(1.0f)
        , innerRadius(0.25f)
        , height(1.9f)
        , stairStep(0.25f)
    {
    }

    HRESULT PlayerBodyComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L"outer_radius"] = String::from(outerRadius);
        componentParameterList[L"inner_radius"] = String::from(innerRadius);
        componentParameterList[L"playerBodyComponent.height"] = String::from(height);
        componentParameterList[L"stair_step"] = String::from(stairStep);
        return S_OK;
    }

    HRESULT PlayerBodyComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"outer_radius", outerRadius, String::to<float>);
        setParameter(componentParameterList, L"inner_radius", innerRadius, String::to<float>);
        setParameter(componentParameterList, L"playerBodyComponent.height", height, String::to<float>);
        setParameter(componentParameterList, L"stair_step", stairStep, String::to<float>);
        return S_OK;
    }

    class PlayerBodyImplementation : public ContextUserMixin
        , public ComponentMixin<PlayerBodyComponent>
    {
    public:
        PlayerBodyImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(PlayerBodyImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
            END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"player_body";
        }
    };

    REGISTER_CLASS(PlayerBodyImplementation)
}; // namespace Gek