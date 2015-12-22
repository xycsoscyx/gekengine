#include "GEK\Newton\RigidBody.h"
#include "GEK\Newton\NewtonEntity.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Matrix4x4.h"
#include <Newton.h>
#include <dNewton.h>
#include <dNewtonCollision.h>
#include <dNewtonDynamicBody.h>

namespace Gek
{
    static const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);

    class RigidNewtonBody : public UnknownMixin
        , public dNewtonDynamicBody
        , public NewtonEntity
    {
    private:
        Entity *entity;

    public:
        RigidNewtonBody(dNewton *newton, const dNewtonCollision* const newtonCollision, Entity *entity,
            const TransformComponent &transformComponent,
            const MassComponent &massComponent)
            : dNewtonDynamicBody(newton, massComponent, newtonCollision, nullptr, Math::Float4x4(transformComponent.rotation, transformComponent.position).data, NULL)
            , entity(entity)
        {
            NewtonBodySetUserData(GetNewtonBody(), this);
        }

        ~RigidNewtonBody(void)
        {
        }

        // NewtonEntity
        STDMETHODIMP_(Entity *) getEntity(void) const
        {
            return entity;
        }

        STDMETHODIMP_(NewtonBody *) getNewtonBody(void) const
        {
            return GetNewtonBody();
        }

        // dNewtonBody
        void OnBodyTransform(const dFloat* const newtonMatrix, int threadHandle)
        {
            REQUIRE_VOID_RETURN(entity);

            Math::Float4x4 matrix(newtonMatrix);
            auto &transformComponent = entity->getComponent<TransformComponent>();
            transformComponent.position = matrix.translation;
            transformComponent.rotation = matrix;
        }

        // dNewtonDynamicBody
        void OnForceAndTorque(dFloat frameTime, int threadHandle)
        {
            REQUIRE_VOID_RETURN(entity);

            float mass = entity->getComponent<MassComponent>();
            AddForce((Gravity * mass).data);
        }
    };

    NewtonEntity *createRigidBody(dNewton *newton, const dNewtonCollision* const newtonCollision, Entity *entity, const TransformComponent &transformComponent, const MassComponent &massComponent)
    {
        return new RigidNewtonBody(newton, newtonCollision, entity, transformComponent, massComponent);
    }

    RigidBodyComponent::RigidBodyComponent(void)
    {
    }

    HRESULT RigidBodyComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L""] = shape;
        componentParameterList[L"surface"] = surface;
        return S_OK;
    }

    HRESULT RigidBodyComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", shape, [](LPCWSTR value) -> LPCWSTR { return value; });
        setParameter(componentParameterList, L"surface", surface, [](LPCWSTR value) -> LPCWSTR { return value; });
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