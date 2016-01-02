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

#define PLAYER_CONTROLLER_MAX_CONTACTS	    32
#define PLAYER_MIN_RESTRAINING_DISTANCE	    1.0e-2f
#define PLAYER_EPSILON                      1.0e-5f
#define PLAYER_EPSILON_SQUARED              (PLAYER_EPSILON * PLAYER_EPSILON)
#define D_DESCRETE_MOTION_STEPS				8
#define D_PLAYER_MAX_INTERGRATION_STEPS		8
#define D_PLAYER_MAX_SOLVER_ITERATIONS		16
#define D_PLAYER_CONTACT_SKIN_THICKNESS		0.025f

namespace Gek
{
    static const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);

    class CustomControllerConvexCastPreFilter
    {
    protected:
        const NewtonBody* sourceBody;

    public:
        CustomControllerConvexCastPreFilter(const NewtonBody *sourceBody = nullptr)
            :sourceBody(sourceBody)
        {
        }

        ~CustomControllerConvexCastPreFilter()
        {
        }

        virtual unsigned Prefilter(const NewtonBody* const body)
        {
            const NewtonCollision* const collision = NewtonBodyGetCollision(body);
            return NewtonCollisionGetMode(collision);
        }

        static unsigned Prefilter(const NewtonBody* const body, const NewtonCollision* const, void* const userData)
        {
            CustomControllerConvexCastPreFilter* const filter = (CustomControllerConvexCastPreFilter*)userData;
            return ((body != filter->sourceBody) ? filter->Prefilter(body) : 0);
        }
    };

    class CustomControllerConvexRayFilter : public CustomControllerConvexCastPreFilter
    {
    public:
        Math::Float3 hitPoint;
        Math::Float3 hitNormal;
        const NewtonBody* hitBody;
        const NewtonCollision* hitShape;
        long long collisionIdentifier;
        float intersectionParam;

    public:
        CustomControllerConvexRayFilter(const NewtonBody* const sourceBody)
            : CustomControllerConvexCastPreFilter(sourceBody)
            , hitBody(NULL)
            , hitShape(NULL)
            , collisionIdentifier(0)
            , intersectionParam(1.2f)
        {
        }

        static float Filter(const NewtonBody* const body, const NewtonCollision* const shapeHit, const float* const hitContact, const float* const hitNormal, long long collisionID, void* const userData, float intersectParam)
        {
            CustomControllerConvexRayFilter* const filter = (CustomControllerConvexRayFilter*)userData;
            if (intersectParam < filter->intersectionParam)
            {
                filter->hitBody = body;
                filter->hitShape = shapeHit;
                filter->collisionIdentifier = collisionID;
                filter->intersectionParam = intersectParam;
                filter->hitPoint = *(Math::Float3 *)hitContact;
                filter->hitNormal = *(Math::Float3 *)hitNormal;
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
    State *createJumpedState(PlayerNewtonBody *playerBody);

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
        NewtonWorld *newtonWorld;
        NewtonBody *newtonBody;
        Entity *entity;
        State *currentState;
        float headingAngle;
        PlayerBodyComponent &playerBodyComponent;
        TransformComponent &transformComponent;
        MassComponent &massComponent;

        Math::Float3 movementUpNormal;
        Math::Float3 movementForwardNormal;
        Math::Float3 groundNormal;
        Math::Float3 groundVelocity;
        float maximumSlope;
        float sphereCastOrigin;
        float restrainingDistance;
        bool isJumping;

        NewtonCollision *newtonCastingShape;
        NewtonCollision *newtonSupportShape;
        NewtonCollision *newtonUpperBodyShape;

        bool IsInFreeFall() const
        {
            return isJumping;
        }

        void SetRestrainingDistance(float distance)
        {
            restrainingDistance = std::max(std::abs(distance), PLAYER_MIN_RESTRAINING_DISTANCE);
        }

        void SetClimbSlope(float slopeInRadians)
        {
            maximumSlope = std::cos(std::abs(slopeInRadians));
        }

        Math::Quaternion IntegrateOmega(const Math::Quaternion &rotation, const Math::Float3& omega, float frameTime) const
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

        Math::Float3 CalculateAverageOmega(const Math::Quaternion &_quaternion0, const Math::Quaternion &quaternion1, float inverseFrameTime) const
        {
            Math::Quaternion quaternion0(_quaternion0);
            if (quaternion0.dot(quaternion1) < 0.0f)
            {
                quaternion0 *= -1.0f;
            }

            Math::Quaternion deltaQuaternion(quaternion0.getInverse() * quaternion1);
            Math::Float3 omegaDirection(deltaQuaternion.x, deltaQuaternion.y, deltaQuaternion.z);

            float directionMagnitudeSquared = omegaDirection.dot(omegaDirection);
            if (directionMagnitudeSquared	< PLAYER_EPSILON_SQUARED)
            {
                return Math::Float3(0.0f);
            }

            float inverseDirectionMagnitude = (1.0f / std::sqrt(directionMagnitudeSquared));
            float directionMagnitude = (directionMagnitudeSquared * inverseDirectionMagnitude);

            float omegaMagnitude = (2.0f * std::atan2(directionMagnitude, deltaQuaternion.w) * inverseFrameTime);
            return (omegaDirection * (inverseDirectionMagnitude * omegaMagnitude));
        }

        Math::Float3 CalculateDesiredOmega(float frameTime) const
        {
            float playerRotationData[4];
            NewtonBodyGetRotation(newtonBody, playerRotationData);
            Math::Quaternion targetRotation(movementUpNormal, headingAngle);
            return CalculateAverageOmega({ playerRotationData[1], playerRotationData[2], playerRotationData[3], playerRotationData[0] }, targetRotation, (0.5f / frameTime));
        }

        Math::Float3 CalculateDesiredVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed, float frameTime) const
        {
            Math::Float4x4 matrix;
            NewtonBodyGetMatrix(newtonBody, matrix.data);

            Math::Float3 upDirection(matrix * movementUpNormal);
            Math::Float3 forwardDirection(matrix * movementForwardNormal);
            Math::Float3 rightDirection(forwardDirection.cross(upDirection));

            Math::Float3 desiredVelocity;
            if ((verticalSpeed <= 0.0f) && (groundNormal.dot(groundNormal) > 0.0f))
            {
                // plane is supported by a ground plane, apply the player input desiredVelocity
                if (groundNormal.dot(upDirection) >= maximumSlope)
                {
                    // player is in a legal slope, he is in full control of his movement
                    Math::Float3 bodyVelocity;
                    NewtonBodyGetVelocity(newtonBody, bodyVelocity.data);
                    desiredVelocity = ((upDirection * bodyVelocity.dot(upDirection)) + (Gravity * frameTime) + (forwardDirection * forwardSpeed) + (rightDirection * lateralSpeed) + (upDirection * verticalSpeed));
                    desiredVelocity += (groundVelocity - (upDirection * upDirection.dot(groundVelocity)));

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
                    desiredVelocity += (upDirection * verticalSpeed);
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
                desiredVelocity += (upDirection * verticalSpeed);
                desiredVelocity += (Gravity * frameTime);
            }

            return desiredVelocity;
        }

        float CalculateContactKinematics(const Math::Float3& velocity, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
        {
            Math::Float3 contactVelocity;
            if (contactInfo->m_hitBody)
            {
                NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, contactVelocity.data);
            }

            const float restitution = 0.0f;
            Math::Float3 normal(contactInfo->m_normal);
            float reboundVelocityMagnitude = -((velocity - contactVelocity).dot(normal) * (1.0f + restitution));
            return ((reboundVelocityMagnitude > 0.0f) ? reboundVelocityMagnitude : 0.0f);
        }

        void UpdateGroundPlane(Math::Float4x4& matrix, const Math::Float4x4& castMatrix, const Math::Float3& destination, int threadHandle)
        {
            CustomControllerConvexRayFilter filter(newtonBody);
            NewtonWorldConvexRayCast(newtonWorld, newtonCastingShape, castMatrix.data, destination.data, CustomControllerConvexRayFilter::Filter, &filter, CustomControllerConvexCastPreFilter::Prefilter, threadHandle);

            groundNormal.set(0.0f);
            groundVelocity.set(0.0f);
            if (filter.hitBody)
            {
                isJumping = false;
                Math::Float3 supportPoint(castMatrix.translation + ((destination - castMatrix.translation) * (filter.intersectionParam)));
                groundNormal = filter.hitNormal;
                NewtonBodyGetPointVelocity(filter.hitBody, supportPoint.data, groundVelocity.data);
                matrix.translation = supportPoint;
                matrix.tw = 1.0f;
            }
        }

    public:
        bool isInFreeFall(void)
        {
            return isJumping;
        }

        void setPlayerVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed, float frameTime)
        {
            Math::Float3 omega(CalculateDesiredOmega(frameTime));
            NewtonBodySetOmega(newtonBody, omega.data);

            Math::Float3 velocity(CalculateDesiredVelocity(forwardSpeed, lateralSpeed, verticalSpeed, frameTime));
            NewtonBodySetVelocity(newtonBody, velocity.data);

            if (verticalSpeed > 0.0f)
            {
                isJumping = true;
            }
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
            , currentState(createIdleState(this))
            , headingAngle(0.0f)
            , playerBodyComponent(playerBodyComponent)
            , transformComponent(transformComponent)
            , massComponent(massComponent)
            , isJumping(false)
            , movementUpNormal(0.0f, 1.0f, 0.0f)
            , movementForwardNormal(0.0f, 0.0f, -1.0f)
        {
            Math::Float4x4 localAxis;
            localAxis.setLookAt(movementForwardNormal, movementUpNormal);

            SetRestrainingDistance(0.0f);

            SetClimbSlope(45.0f * 3.1416f / 180.0f);

            groundNormal.set(0.0f);
            groundVelocity.set(0.0f);

            const int numberOfSteps = 12;
            Math::Float3 convexPoints[2][numberOfSteps];

            // create an inner thin cylinder
            Math::Float3 point0(0.0f, playerBodyComponent.innerRadius, 0.0f);
            Math::Float3 point1(playerBodyComponent.height, playerBodyComponent.innerRadius, 0.0f);
            for (int index = 0; index < numberOfSteps; index++)
            {
                Math::Float4x4 rotation;
                rotation.setPitchRotation(index * 2.0f * 3.141592f / numberOfSteps);
                convexPoints[0][index] = (localAxis * (rotation * point0));
                convexPoints[1][index] = (localAxis * (rotation * point1));
            }

            NewtonCollision* const supportShape = NewtonCreateConvexHull(newtonWorld, (numberOfSteps * 2), convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, NULL);

            // create the outer thick cylinder
            Math::Float4x4 outerShapeMatrix(localAxis);
            float capsuleHeight = (playerBodyComponent.height - playerBodyComponent.stairStep);
            sphereCastOrigin = ((capsuleHeight * 0.5f) + playerBodyComponent.stairStep);
            outerShapeMatrix.translation = (outerShapeMatrix[0] * sphereCastOrigin).xyz;
            outerShapeMatrix.tw = 1.0f;
            NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.5f, 0, outerShapeMatrix.data);
            NewtonCollisionSetScale(bodyCapsule, capsuleHeight, (playerBodyComponent.outerRadius * 4.0f), (playerBodyComponent.outerRadius * 4.0f));

            // compound collision player controller
            NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
            NewtonCompoundCollisionBeginAddRemove(playerShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
            NewtonCompoundCollisionEndAddRemove(playerShape);

            // create the kinematic body
            newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, Math::Float4x4().data);

            // players must have weight, otherwise they are infinitely strong when they collide
            NewtonCollision* const shape = NewtonBodyGetCollision(newtonBody);
            NewtonBodySetMassProperties(newtonBody, massComponent, shape);

            // make the body collidable with other dynamics bodies, by default
            NewtonBodySetCollidable(newtonBody, true);

            float castHeight = (capsuleHeight * 0.4f);
            float castRadius = (((playerBodyComponent.innerRadius * 0.5f) > 0.05f) ? (playerBodyComponent.innerRadius * 0.5f) : 0.05f);

            Math::Float3 quaternion0(0.0f, castRadius, 0.0f);
            Math::Float3 quaternion1(castHeight, castRadius, 0.0f);
            for (int index = 0; index < numberOfSteps; index++)
            {
                Math::Float4x4 rotation;
                rotation.setPitchRotation(index * 2.0f * 3.141592f / numberOfSteps);
                convexPoints[0][index] = (localAxis * (rotation * quaternion0));
                convexPoints[1][index] = (localAxis * (rotation * quaternion1));
            }

            newtonCastingShape = NewtonCreateConvexHull(newtonWorld, (numberOfSteps * 2), convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, NULL);
            newtonSupportShape = NewtonCompoundCollisionGetCollisionFromNode(shape, NewtonCompoundCollisionGetNodeByIndex(shape, 0));
            newtonUpperBodyShape = NewtonCompoundCollisionGetCollisionFromNode(shape, NewtonCompoundCollisionGetNodeByIndex(shape, 1));

            NewtonDestroyCollision(bodyCapsule);
            NewtonDestroyCollision(supportShape);
            NewtonDestroyCollision(playerShape);

            isJumping = false;

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
            State *newState = (currentState ? currentState->onUpdate(frameTime) : nullptr);
            if (newState)
            {
                currentState->onExit();
                delete currentState;
                currentState = newState;
                newState->onEnter();
            }
        }

        STDMETHODIMP_(void) onPostUpdate(float frameTime, int threadHandle)
        {
            // get the body motion state 
            Math::Float4x4 matrix;
            NewtonBodyGetMatrix(newtonBody, matrix.data);

            Math::Float3 omega;
            NewtonBodyGetOmega(newtonBody, omega.data);

            // integrate body angular velocity
            Math::Quaternion bodyRotation(IntegrateOmega(matrix.getQuaternion(), omega, frameTime));
            matrix = bodyRotation.getMatrix(matrix.translation);

            // integrate linear velocity
            Math::Float3 velocity;
            NewtonBodyGetVelocity(newtonBody, velocity.data);
            float normalizedTimeLeft = 1.0f;
            float step = (frameTime * velocity.getLength());
            float descreteframeTime = (frameTime * (1.0f / D_DESCRETE_MOTION_STEPS));
            int previousContactCount = 0;
            CustomControllerConvexCastPreFilter castFilterData(newtonBody);
            NewtonWorldConvexCastReturnInfo previousInfo[PLAYER_CONTROLLER_MAX_CONTACTS];

            Math::Float3 upDirection(matrix * movementUpNormal);

            Math::Float3 scale;
            NewtonCollisionGetScale(newtonUpperBodyShape, &scale.x, &scale.y, &scale.z);
            //const float radius = (playerBodyComponent.outerRadius * 4.0f);
            const float radius = ((playerBodyComponent.outerRadius + restrainingDistance) * 4.0f);
            NewtonCollisionSetScale(newtonUpperBodyShape, playerBodyComponent.height - playerBodyComponent.stairStep, radius, radius);

            NewtonWorldConvexCastReturnInfo upConstraint;
            memset(&upConstraint, 0, sizeof(upConstraint));
            upConstraint.m_normal[0] = movementUpNormal.x;
            upConstraint.m_normal[1] = movementUpNormal.y;
            upConstraint.m_normal[2] = movementUpNormal.z;
            upConstraint.m_normal[3] = 0.0f;

            for (int step = 0; ((step < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedTimeLeft > PLAYER_EPSILON)); step++)
            {
                if (velocity.dot(velocity) < 1.0e-6f)
                {
                    break;
                }

                float timeToImpact;
                NewtonWorldConvexCastReturnInfo info[PLAYER_CONTROLLER_MAX_CONTACTS];
                Math::Float3 destinationPosition(matrix.translation + (velocity * frameTime));
                int contactCount = NewtonWorldConvexCast(newtonWorld, matrix.data, destinationPosition.data, newtonUpperBodyShape, &timeToImpact, &castFilterData, CustomControllerConvexCastPreFilter::Prefilter, info, ARRAYSIZE(info), threadHandle);
                if (contactCount)
                {
                    matrix.translation += (velocity * (timeToImpact * frameTime));
                    if (timeToImpact > 0.0f)
                    {
                        matrix.translation -= (velocity * (D_PLAYER_CONTACT_SKIN_THICKNESS / velocity.getLength()));
                    }

                    normalizedTimeLeft -= timeToImpact;

                    float speed[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                    float bounceSpeed[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
                    Math::Float3 bounceNormal[PLAYER_CONTROLLER_MAX_CONTACTS * 2];
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

                    int count = 0;
                    if (!isJumping)
                    {
                        upConstraint.m_point[0] = matrix.translation.x;
                        upConstraint.m_point[1] = matrix.translation.y;
                        upConstraint.m_point[2] = matrix.translation.z;
                        upConstraint.m_point[3] = matrix.tw;

                        speed[count] = 0.0f;
                        bounceNormal[count].set(upConstraint.m_normal);
                        bounceSpeed[count] = CalculateContactKinematics(velocity, &upConstraint);
                        count++;
                    }

                    for (int index = 0; index < contactCount; index++)
                    {
                        speed[count] = 0.0f;
                        bounceNormal[count].set(info[index].m_normal);
                        bounceSpeed[count] = CalculateContactKinematics(velocity, &info[index]);
                        count++;
                    }

                    for (int index = 0; index < previousContactCount; index++)
                    {
                        speed[count] = 0.0f;
                        bounceNormal[count] = Math::Float3(previousInfo[index].m_normal);
                        bounceSpeed[count] = CalculateContactKinematics(velocity, &previousInfo[index]);
                        count++;
                    }

                    float residual = 10.0f;
                    Math::Float3 auxiliaryBounceVelocity(0.0f, 0.0f, 0.0f);
                    for (int index = 0; ((index < D_PLAYER_MAX_SOLVER_ITERATIONS) && (residual > 1.0e-3f)); index++)
                    {
                        residual = 0.0f;
                        for (int bound = 0; bound < count; bound++)
                        {
                            Math::Float3 normal(bounceNormal[bound]);
                            float value = (bounceSpeed[bound] - normal.dot(auxiliaryBounceVelocity));
                            float delta = (speed[bound] + value);
                            if (delta < 0.0f)
                            {
                                value = 0.0f;
                                delta = 0.0f;
                            }

                            if (std::abs(value) > residual)
                            {
                                residual = std::abs(value);
                            }

                            auxiliaryBounceVelocity += (normal * (delta - speed[bound]));
                            speed[bound] = delta;
                        }
                    }

                    Math::Float3 velocityStep(0.0f);
                    for (int index = 0; index < count; index++)
                    {
                        Math::Float3 normal(bounceNormal[index]);
                        velocityStep += (normal * speed[index]);
                    }

                    velocity += velocityStep;
                    float velocityMagnitudeSquared = velocityStep.dot(velocityStep);
                    if (velocityMagnitudeSquared < 1.0e-6f)
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
                    matrix.tw = 1.0f;
                    break;
                }
            }

            NewtonCollisionSetScale(newtonUpperBodyShape, scale.x, scale.y, scale.z);

            // determine if player is standing on some plane
            Math::Float4x4 supportMatrix(matrix);
            supportMatrix.translation += (upDirection * sphereCastOrigin);
            if (isJumping)
            {
                Math::Float3 destination(matrix.translation);
                UpdateGroundPlane(matrix, supportMatrix, destination, threadHandle);
            }
            else
            {
                step = std::abs(upDirection.dot(velocity * frameTime));
                float castDist = ((groundNormal.dot(groundNormal) > 0.0f) ? playerBodyComponent.stairStep : step);
                Math::Float3 destination(matrix.translation - (upDirection * (castDist * 2.0f)));
                UpdateGroundPlane(matrix, supportMatrix, destination, threadHandle);
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
            playerBody->setPlayerVelocity(0.0f, 0.0f, 0.0f, frameTime);
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
            playerBody->setPlayerVelocity(0.0f, 0.0f, 0.0f, frameTime);
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

            float lateralSpeed = (((moveForward ? -1.0f : 0.0f) + (moveBackward ? 1.0f : 0.0f)) * 5.0f);
            float strafeSpeed = (((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f);
            playerBody->setPlayerVelocity(lateralSpeed, strafeSpeed, 0.0f, frameTime);
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
    public:
        JumpingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            REQUIRE_RETURN(playerBody, nullptr);
            playerBody->setPlayerVelocity(0.0f, 0.0f, 10.0f, frameTime);
            return createJumpedState(playerBody);
        }
    };

    class JumpedState : public PlayerStateMixin
    {
    public:
        JumpedState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            REQUIRE_RETURN(playerBody, nullptr);

            playerBody->setPlayerVelocity(0.0f, 0.0f, 0.0f, frameTime);
            if (!playerBody->isInFreeFall())
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

    State *createJumpedState(PlayerNewtonBody *playerBody)
    {
        return new JumpedState(playerBody);
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