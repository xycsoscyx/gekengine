#include "GEK\Newton\RigidBody.h"
#include "GEK\Newton\NewtonEntity.h"
#include "GEK\Newton\NewtonProcessor.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include <Newton.h>

namespace Gek
{
    class RigidNewtonBody : public UnknownMixin
        , public NewtonEntity
    {
    private:
        NewtonProcessor *newtonProcessor;

        Entity *entity;
        NewtonBody *newtonBody;
        TransformComponent &transformComponent;
        MassComponent &massComponent;

    public:
        RigidNewtonBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Entity *entity,
            TransformComponent &transformComponent,
            MassComponent &massComponent)
            : newtonProcessor(static_cast<NewtonProcessor *>(NewtonWorldGetUserData(newtonWorld)))
            , entity(entity)
            , newtonBody(NewtonCreateDynamicBody(newtonWorld, newtonCollision, transformComponent.getMatrix().data))
            , transformComponent(transformComponent)
            , massComponent(massComponent)
        {
            GEK_REQUIRE_VOID_RETURN(newtonBody);
            NewtonBodySetMassProperties(newtonBody, massComponent, newtonCollision);
            NewtonBodySetMatrix(newtonBody, transformComponent.getMatrix().data);
            NewtonBodySetUserData(newtonBody, dynamic_cast<NewtonEntity *>(this));
        }

        ~RigidNewtonBody(void)
        {
            NewtonDestroyBody(newtonBody);
        }

        // IUnknown
        BEGIN_INTERFACE_LIST(RigidNewtonBody)
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

        STDMETHODIMP_(UINT32) getSurface(const Math::Float3 &position, const Math::Float3 &normal)
        {
            return 0;
        }

        STDMETHODIMP_(void) onPreUpdate(dFloat frameTime, int threadHandle)
        {
            NewtonCollisionSetScale(NewtonBodyGetCollision(newtonBody), transformComponent.scale.x, transformComponent.scale.y, transformComponent.scale.z);

            Math::Float3 gravity(newtonProcessor->getGravity(transformComponent.position));
            NewtonBodyAddForce(newtonBody, (gravity * (float)massComponent).data);
        }

        STDMETHODIMP_(void) onSetTransform(const dFloat* const matrixData, int threadHandle)
        {
            Math::Float4x4 matrix(matrixData);
            transformComponent.position = matrix.translation;
            transformComponent.rotation = matrix.getQuaternion();
        }
    };

    NewtonEntity *createRigidBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Entity *entity, TransformComponent &transformComponent, MassComponent &massComponent)
    {
        return new RigidNewtonBody(newtonWorld, newtonCollision, entity, transformComponent, massComponent);
    }

    RigidBodyComponent::RigidBodyComponent(void)
    {
    }

    HRESULT RigidBodyComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, shape);
        saveParameter(componentData, L"surface", surface);
        return S_OK;
    }

    HRESULT RigidBodyComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, shape);
        loadParameter(componentData, L"surface", surface);
        return S_OK;
    }

    class RigidBodyImplementation : public ContextUserMixin
        , public ComponentMixin<RigidBodyComponent>
    {
    public:
        RigidBodyImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(RigidBodyImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"rigid_body";
        }
    };

    REGISTER_CLASS(RigidBodyImplementation)
}; // namespace Gek