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
#include "GEK\Shape\Plane.h"
#include <Newton.h>

#define PLAYER_CONTROLLER_MAX_CONTACTS	32
#define PLAYER_MIN_RESTRAINING_DISTANCE	1.0e-2f
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
        const NewtonBody* m_me;

    public:
        CustomControllerConvexCastPreFilter()
            :m_me(NULL)
        {
        }

        CustomControllerConvexCastPreFilter(const NewtonBody* const me)
            : m_me(me)
        {
        }


        ~CustomControllerConvexCastPreFilter()
        {
        }

        virtual unsigned Prefilter(const NewtonBody* const body, const NewtonCollision* const myCollision)
        {
            const NewtonCollision* const collision = NewtonBodyGetCollision(body);
            unsigned retValue = NewtonCollisionGetMode(collision);
            return retValue;
        }

        static unsigned Prefilter(const NewtonBody* const body, const NewtonCollision* const myCollision, void* const userData)
        {
            CustomControllerConvexCastPreFilter* const filter = (CustomControllerConvexCastPreFilter*)userData;
            return (body != filter->m_me) ? filter->Prefilter(body, myCollision) : 0;
        }
    };

    class CustomControllerConvexRayFilter : public CustomControllerConvexCastPreFilter
    {
    public:
        Math::Float3 m_hitContact;
        Math::Float3 m_hitNormal;
        const NewtonBody* m_hitBody;
        const NewtonCollision* m_shapeHit;
        long long m_collisionID;
        float m_intersectParam;

    public:
        CustomControllerConvexRayFilter(const NewtonBody* const me)
            : CustomControllerConvexCastPreFilter()
            , m_hitBody(NULL)
            , m_shapeHit(NULL)
            , m_collisionID(0)
            , m_intersectParam(1.2f)
        {
            m_me = me;
        }

        static float Filter(const NewtonBody* const body, const NewtonCollision* const shapeHit, const float* const hitContact, const float* const hitNormal, long long collisionID, void* const userData, float intersectParam)
        {
            CustomControllerConvexRayFilter* const filter = (CustomControllerConvexRayFilter*)userData;
            if (intersectParam < filter->m_intersectParam)
            {
                filter->m_hitBody = body;
                filter->m_shapeHit = shapeHit;
                filter->m_collisionID = collisionID;
                filter->m_intersectParam = intersectParam;
                filter->m_hitContact.set(hitContact[0], hitContact[1], hitContact[2]);
                filter->m_hitNormal.set(hitNormal[0], hitNormal[1], hitNormal[2]);
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
    private:
        IUnknown *actionProvider;
        NewtonWorld *newtonWorld;
        NewtonBody *newtonBody;
        Entity *entity;
        State *currentState;
        float heading;
        PlayerBodyComponent &playerBodyComponent;
        TransformComponent &transformComponent;
        MassComponent &massComponent;

        Math::Float3 m_upVector;
        Math::Float3 m_frontVector;
        Shape::Plane m_groundPlane;
        Math::Float3 m_groundVelocity;
        float m_outerRadio;
        float m_innerRadio;
        float m_height;
        float m_stairStep;
        float m_maxSlope;
        float m_sphereCastOrigin;
        float m_restrainingDistance;
        bool m_isJumping;
        NewtonCollision* m_castingShape;
        NewtonCollision* m_supportShape;
        NewtonCollision* m_upperBodyShape;

        float GetHigh() const
        {
            return m_height;
        }

        const Math::Float3 GetUpDir() const
        {
            return m_upVector;
        }
        const Shape::Plane& GetGroundPlane() const
        {
            return m_groundPlane;
        }

        bool IsInFreeFall() const
        {
            return m_isJumping;
        }

        void SetRestrainingDistance(float distance)
        {
            m_restrainingDistance = std::max(std::abs(distance), float(PLAYER_MIN_RESTRAINING_DISTANCE));
        }

        void SetClimbSlope(float slopeInRadians)
        {
            m_maxSlope = std::cos(std::abs(slopeInRadians));
        }

        Math::Quaternion IntegrateOmega(const Math::Quaternion &rotation, const Math::Float3& omega, float frameTime) const
        {
            Math::Quaternion integratedRotation(rotation);

            // this is correct
            float omegaMag2 = omega.dot(omega);
            const float errAngle = 0.0125f * 3.141592f / 180.0f;
            const float errAngle2 = errAngle * errAngle;
            if (omegaMag2 > errAngle2)
            {
                float invOmegaMag = 1.0f / std::sqrt(omegaMag2);
                Math::Float3 omegaAxis(omega * (invOmegaMag));
                float omegaAngle = invOmegaMag * omegaMag2 * frameTime;
                Math::Quaternion deltaRotation(Math::Quaternion::createRotation(omegaAxis, omegaAngle));
                integratedRotation = (rotation * deltaRotation);
                integratedRotation.x *= (1.0f / std::sqrt(rotation.dot(rotation)));
                integratedRotation.y *= (1.0f / std::sqrt(rotation.dot(rotation)));
                integratedRotation.z *= (1.0f / std::sqrt(rotation.dot(rotation)));
                integratedRotation.w *= (1.0f / std::sqrt(rotation.dot(rotation)));
            }

            return integratedRotation;
        }

        Math::Float3 CalcAverageOmega(const Math::Quaternion &q00, const Math::Quaternion &q1, float invdt) const
        {
            Math::Quaternion q0(q00);
            if (q0.dot(q1) < 0.0f)
            {
                q0.x *= -1.0f;
                q0.y *= -1.0f;
                q0.z *= -1.0f;
                q0.w *= -1.0f;
            }

            Math::Quaternion dq(q0.getInverse() * q1);
            Math::Float3 omegaDir(dq.x, dq.y, dq.z);

            float dirMag2 = omegaDir.dot(omegaDir);
            if (dirMag2	< float(float(1.0e-5f) * float(1.0e-5f)))
            {
                return Math::Float3(0.0f, 0.0f, 0.0f);
            }

            float dirMagInv = float(1.0f) / std::sqrt(dirMag2);
            float dirMag = dirMag2 * dirMagInv;

            float omegaMag = float(2.0f) * std::atan2(dirMag, dq.w) * invdt;
            return omegaDir * (dirMagInv * omegaMag);
        }

        Math::Float3 CalculateDesiredOmega(float headingAngle, float frameTime) const
        {
            Math::Quaternion playerRotation;
            Math::Quaternion targetRotation(Math::Quaternion::createRotation(m_upVector, headingAngle));
            NewtonBodyGetRotation(newtonBody, playerRotation.data);
            return CalcAverageOmega(playerRotation, targetRotation, 0.5f / frameTime);
        }

        Math::Float3 CalculateDesiredVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed, const Math::Float3& gravity, float frameTime) const
        {
            Math::Float4x4 matrix;
            NewtonBodyGetMatrix(newtonBody, matrix.data);
            Math::Float3 updir(matrix * m_upVector);
            Math::Float3 frontDir(matrix * m_frontVector);
            Math::Float3 rightDir(frontDir.cross(updir));

            Math::Float3 veloc(0.0f, 0.0f, 0.0f);
            if ((verticalSpeed <= 0.0f) && (m_groundPlane.normal.dot(m_groundPlane.normal) > 0.0f))
            {
                // plane is supported by a ground plane, apply the player input velocity
                if ((m_groundPlane.normal.dot(updir)) >= m_maxSlope) 
                {
                    // player is in a legal slope, he is in full control of his movement
                    Math::Float3 bodyVeloc;
                    NewtonBodyGetVelocity(newtonBody, &bodyVeloc[0]);
                    veloc = updir * bodyVeloc.dot(updir) + gravity * frameTime + frontDir * forwardSpeed + rightDir * lateralSpeed + updir * verticalSpeed;
                    veloc += (m_groundVelocity - updir * updir.dot(m_groundVelocity));

                    float speedLimitMag2 = forwardSpeed * forwardSpeed + lateralSpeed * lateralSpeed + verticalSpeed * verticalSpeed + m_groundVelocity.dot(m_groundVelocity) + 0.1f;
                    float speedMag2 = veloc.dot(veloc);
                    if (speedMag2 > speedLimitMag2)
                    {
                        veloc = veloc * std::sqrt(speedLimitMag2 / speedMag2);
                    }

                    float normalVeloc = m_groundPlane.normal.dot(veloc - m_groundVelocity);
                    if (normalVeloc < 0.0f)
                    {
                        veloc -= m_groundPlane.normal.dot(normalVeloc);
                    }
                }

                else
                {
                    // player is in an illegal ramp, he slides down hill an loses control of his movement 
                    NewtonBodyGetVelocity(newtonBody, &veloc[0]);
                    veloc += updir * verticalSpeed;
                    veloc += gravity * frameTime;
                    float normalVeloc = m_groundPlane.normal.dot(veloc - m_groundVelocity);
                    if (normalVeloc < 0.0f)
                    {
                        veloc -= m_groundPlane.normal.cross(normalVeloc);
                    }
                }
            }
            else
            {
                // player is on free fall, only apply the gravity
                NewtonBodyGetVelocity(newtonBody, &veloc[0]);
                veloc += updir * verticalSpeed;
                veloc += gravity * frameTime;
            }

            return veloc;
        }

        float CalculateContactKinematics(const Math::Float3& veloc, const NewtonWorldConvexCastReturnInfo* const contactInfo) const
        {
            Math::Float3 contactVeloc(0.0f, 0.0f, 0.0f);
            if (contactInfo->m_hitBody)
            {
                NewtonBodyGetPointVelocity(contactInfo->m_hitBody, contactInfo->m_point, &contactVeloc[0]);
            }

            const float restitution = 0.0f;
            Math::Float3 normal(contactInfo->m_normal);
            float reboundVelocMag = -((veloc - contactVeloc).dot(normal)) * (1.0f + restitution);
            return (reboundVelocMag > 0.0f) ? reboundVelocMag : 0.0f;
        }

        void UpdateGroundPlane(Math::Float4x4& matrix, const Math::Float4x4& castMatrix, const Math::Float3& dst, int threadIndex)
        {
            CustomControllerConvexRayFilter filter(newtonBody);
            NewtonWorldConvexRayCast(newtonWorld, m_castingShape, &castMatrix[0][0], dst.data, CustomControllerConvexRayFilter::Filter, &filter, CustomControllerConvexCastPreFilter::Prefilter, threadIndex);

            m_groundPlane.a = m_groundPlane.b = m_groundPlane.c = m_groundPlane.d = 0.0f;
            m_groundVelocity.set(0.0f, 0.0f, 0.0f);

            if (filter.m_hitBody)
            {
                m_isJumping = false;
                Math::Float3 supportPoint(castMatrix.translation + (dst - castMatrix.translation) * (filter.m_intersectParam));
                m_groundPlane.normal = filter.m_hitNormal;
                m_groundPlane.d = -(supportPoint.dot(filter.m_hitNormal));
                NewtonBodyGetPointVelocity(filter.m_hitBody, supportPoint.data, &m_groundVelocity[0]);
                matrix.translation = supportPoint;
                matrix.tw = 1.0f;
            }
        }

    public:
        bool isInFreeFall(void)
        {
            return m_isJumping;
        }

        void setPlayerVelocity(float forwardSpeed, float lateralSpeed, float verticalSpeed, float frameTime)
        {
            Math::Float3 omega(CalculateDesiredOmega(heading, frameTime));
            Math::Float3 veloc(CalculateDesiredVelocity(forwardSpeed, lateralSpeed, verticalSpeed, Gravity, frameTime));

            NewtonBodySetOmega(newtonBody, &omega[0]);
            NewtonBodySetVelocity(newtonBody, &veloc[0]);

            if ((verticalSpeed > 0.0f))
            {
                m_isJumping = true;
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
            , heading(0.0f)
            , playerBodyComponent(playerBodyComponent)
            , transformComponent(transformComponent)
            , massComponent(massComponent)
        {
            Math::Float4x4 localAxis({ 0.0f, 1.0f, 0.0f, 0.0f,
                                       1.0f, 0.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, -1.0f, 0.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f });

            SetRestrainingDistance(0.0f);

            m_outerRadio = playerBodyComponent.outerRadius;
            m_innerRadio = playerBodyComponent.innerRadius;
            m_height = playerBodyComponent.height;
            m_stairStep = playerBodyComponent.stairStep;
            SetClimbSlope(45.0f * 3.1416f / 180.0f);
            m_upVector = localAxis[0].xyz;
            m_frontVector = localAxis[1].xyz;

            m_groundPlane.a = m_groundPlane.b = m_groundPlane.c = m_groundPlane.d = 0.0f;
            m_groundVelocity.set(0.0f, 0.0f, 0.0f);

            const int steps = 12;
            Math::Float3 convexPoints[2][steps];

            // create an inner thin cylinder
            float shapeHigh = m_height;
            Math::Float3 p0(0.0f, m_innerRadio, 0.0f);
            Math::Float3 p1(shapeHigh, m_innerRadio, 0.0f);
            for (int i = 0; i < steps; i++)
            {
                Math::Float4x4 rotation(Math::Float4x4::createPitch(i * 2.0f * 3.141592f / steps));
                convexPoints[0][i] = localAxis * (rotation * p0);
                convexPoints[1][i] = localAxis * (rotation * p1);
            }

            NewtonCollision* const supportShape = NewtonCreateConvexHull(newtonWorld, steps * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, NULL);

            // create the outer thick cylinder
            Math::Float4x4 outerShapeMatrix(localAxis);
            float capsuleHigh = m_height - m_stairStep;
            m_sphereCastOrigin = capsuleHigh * 0.5f + m_stairStep;
            outerShapeMatrix.translation = (outerShapeMatrix[0].xyz * m_sphereCastOrigin);
            outerShapeMatrix.tw = 1.0f;
            NewtonCollision* const bodyCapsule = NewtonCreateCapsule(newtonWorld, 0.25f, 0.5f, 0, &outerShapeMatrix[0][0]);
            NewtonCollisionSetScale(bodyCapsule, capsuleHigh, m_outerRadio * 4.0f, m_outerRadio * 4.0f);

            // compound collision player controller
            NewtonCollision* const playerShape = NewtonCreateCompoundCollision(newtonWorld, 0);
            NewtonCompoundCollisionBeginAddRemove(playerShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, supportShape);
            NewtonCompoundCollisionAddSubCollision(playerShape, bodyCapsule);
            NewtonCompoundCollisionEndAddRemove(playerShape);

            // create the kinematic body
            Math::Float4x4 locationMatrix(Math::Float4x4::createIdentity());
            newtonBody = NewtonCreateKinematicBody(newtonWorld, playerShape, &locationMatrix[0][0]);

            // players must have weight, otherwise they are infinitely strong when they collide
            NewtonCollision* const shape = NewtonBodyGetCollision(newtonBody);
            NewtonBodySetMassProperties(newtonBody, massComponent, shape);

            // make the body collidable with other dynamics bodies, by default
            NewtonBodySetCollidable(newtonBody, true);

            float castHigh = capsuleHigh * 0.4f;
            float castRadio = (m_innerRadio * 0.5f > 0.05f) ? m_innerRadio * 0.5f : 0.05f;
            Math::Float3 q0(0.0f, castRadio, 0.0f);
            Math::Float3 q1(castHigh, castRadio, 0.0f);
            for (int i = 0; i < steps; i++)
            {
                Math::Float4x4 rotation(Math::Float4x4::createPitch(i * 2.0f * 3.141592f / steps));
                convexPoints[0][i] = localAxis * (rotation * q0);
                convexPoints[1][i] = localAxis * (rotation * q1);
            }

            m_castingShape = NewtonCreateConvexHull(newtonWorld, steps * 2, convexPoints[0][0].data, sizeof(Math::Float3), 0.0f, 0, NULL);
            m_supportShape = NewtonCompoundCollisionGetCollisionFromNode(shape, NewtonCompoundCollisionGetNodeByIndex(shape, 0));
            m_upperBodyShape = NewtonCompoundCollisionGetCollisionFromNode(shape, NewtonCompoundCollisionGetNodeByIndex(shape, 1));

            NewtonDestroyCollision(bodyCapsule);
            NewtonDestroyCollision(supportShape);
            NewtonDestroyCollision(playerShape);

            m_isJumping = false;
        }

        ~PlayerNewtonBody(void)
        {
            ObservableMixin::removeObserver(actionProvider, getClass<ActionObserver>());
        }

        // IUnknown
        BEGIN_INTERFACE_LIST(PlayerNewtonBody)
            INTERFACE_LIST_ENTRY_COM(ActionObserver);
        END_INTERFACE_LIST_UNKNOWN

        // NewtonEntity
        STDMETHODIMP_(Entity *) getEntity(void) const
        {
            return entity;
        }

        STDMETHODIMP_(NewtonBody *) getNewtonBody(void) const
        {
            return newtonBody;
        }

        STDMETHODIMP_(void) onPostUpdate(float frameTime, int threadHandle)
        {
            State *newState = (currentState ? currentState->onUpdate(frameTime) : nullptr);
            if (newState)
            {
                currentState->onExit();
                delete currentState;
                currentState = newState;
                newState->onEnter();
            }

            Math::Float4x4 matrix;
            Math::Quaternion bodyRotation;
            Math::Float3 veloc(0.0f, 0.0f, 0.0f);
            Math::Float3 omega(0.0f, 0.0f, 0.0f);

            // get the body motion state 
            NewtonBodyGetMatrix(newtonBody, matrix.data);
            NewtonBodyGetVelocity(newtonBody, veloc.data);
            NewtonBodyGetOmega(newtonBody, omega.data);

            // integrate body angular velocity
            NewtonBodyGetRotation(newtonBody, bodyRotation.data);
            bodyRotation = IntegrateOmega(bodyRotation, omega, frameTime);
            matrix = Math::Float4x4::createMatrix(bodyRotation, matrix.translation);

            // integrate linear velocity
            float normalizedTimeLeft = 1.0f;
            float step = frameTime * std::sqrt(veloc.dot(veloc));
            float descreteTimeStep = frameTime * (1.0f / D_DESCRETE_MOTION_STEPS);
            int prevContactCount = 0;
            CustomControllerConvexCastPreFilter castFilterData(newtonBody);
            NewtonWorldConvexCastReturnInfo prevInfo[PLAYER_CONTROLLER_MAX_CONTACTS];

            Math::Float3 updir(matrix * m_upVector);

            Math::Float3 scale;
            NewtonCollisionGetScale(m_upperBodyShape, &scale.x, &scale.y, &scale.z);

            //const float radio = m_outerRadio * 4.0f;
            const float radio = (m_outerRadio + m_restrainingDistance) * 4.0f;
            NewtonCollisionSetScale(m_upperBodyShape, m_height - m_stairStep, radio, radio);

            NewtonWorldConvexCastReturnInfo upConstratint;
            memset(&upConstratint, 0, sizeof(upConstratint));
            upConstratint.m_normal[0] = m_upVector.x;
            upConstratint.m_normal[1] = m_upVector.y;
            upConstratint.m_normal[2] = m_upVector.z;
            upConstratint.m_normal[3] = 0.0f;

            for (int j = 0; (j < D_PLAYER_MAX_INTERGRATION_STEPS) && (normalizedTimeLeft > 1.0e-5f); j++)
            {
                if ((veloc.dot(veloc)) < 1.0e-6f)
                {
                    break;
                }

                float timetoImpact;
                NewtonWorldConvexCastReturnInfo info[PLAYER_CONTROLLER_MAX_CONTACTS];
                Math::Float3 destPosit(matrix.translation + veloc * frameTime);
                int contactCount = NewtonWorldConvexCast(newtonWorld, &matrix[0][0], &destPosit[0], m_upperBodyShape, &timetoImpact, &castFilterData, CustomControllerConvexCastPreFilter::Prefilter, info, sizeof(info) / sizeof(info[0]), threadHandle);
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
                        for (int j = 0; j < i; j++)
                        {
                            Math::Float3 n1(info[j].m_normal);
                            if ((n0.dot(n1)) > 0.9999f)
                            {
                                info[i] = info[contactCount - 1];
                                i--;
                                contactCount--;
                                break;
                            }
                        }
                    }

                    int count = 0;
                    if (!m_isJumping)
                    {
                        upConstratint.m_point[0] = matrix.translation.x;
                        upConstratint.m_point[1] = matrix.translation.y;
                        upConstratint.m_point[2] = matrix.translation.z;
                        upConstratint.m_point[3] = 0.0f;

                        speed[count] = 0.0f;
                        bounceNormal[count] = Math::Float3(upConstratint.m_normal);
                        bounceSpeed[count] = CalculateContactKinematics(veloc, &upConstratint);
                        count++;
                    }

                    for (int i = 0; i < contactCount; i++)
                    {
                        speed[count] = 0.0f;
                        bounceNormal[count] = Math::Float3(info[i].m_normal);
                        bounceSpeed[count] = CalculateContactKinematics(veloc, &info[i]);
                        count++;
                    }

                    for (int i = 0; i < prevContactCount; i++)
                    {
                        speed[count] = 0.0f;
                        bounceNormal[count] = Math::Float3(prevInfo[i].m_normal);
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

                    Math::Float3 velocStep(0.0f, 0.0f, 0.0f);
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
                        matrix.translation += veloc * (advanceTime);
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

            NewtonCollisionSetScale(m_upperBodyShape, scale.x, scale.y, scale.z);

            // determine if player is standing on some plane
            Math::Float4x4 supportMatrix(matrix);
            supportMatrix.translation += updir * (m_sphereCastOrigin);
            if (m_isJumping)
            {
                Math::Float3 dst(matrix.translation);
                UpdateGroundPlane(matrix, supportMatrix, dst, threadHandle);
            }
            else
            {
                step = std::abs(updir.dot(veloc * (frameTime)));
                float castDist = (m_groundPlane.normal.dot(m_groundPlane.normal) > 0.0f) ? m_stairStep : step;
                Math::Float3 dst(matrix.translation - updir.cross(castDist * 2.0f));
                UpdateGroundPlane(matrix, supportMatrix, dst, threadHandle);
            }

            // set player velocity, position and orientation
            NewtonBodySetVelocity(newtonBody, &veloc[0]);
            NewtonBodySetMatrix(newtonBody, &matrix[0][0]);
        }

        // ActionObserver
        STDMETHODIMP_(void) onAction(LPCWSTR name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"turn") == 0)
            {
                heading += (param.value * 0.01f);
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

            float lateralSpeed = ((moveForward ? -1.0f : 0.0f) + (moveBackward ? 1.0f : 0.0f)) * 5.0f;
            float strafeSpeed = ((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f;
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

            playerBody->setPlayerVelocity(0.0f, 0.0f, 50.0f, frameTime);

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

            if (playerBody->isInFreeFall())
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
        componentParameterList[L"height"] = String::from(height);
        componentParameterList[L"stair_step"] = String::from(stairStep);
        return S_OK;
    }

    HRESULT PlayerBodyComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"outer_radius", outerRadius, String::to<float>);
        setParameter(componentParameterList, L"inner_radius", innerRadius, String::to<float>);
        setParameter(componentParameterList, L"height", height, String::to<float>);
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