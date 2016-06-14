#include "GEK\Newton\RigidBody.h"
#include "GEK\Newton\NewtonEntity.h"
#include "GEK\Newton\NewtonProcessor.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include <Newton.h>

namespace Gek
{
    class RigidNewtonBody
        : public NewtonEntity
    {
    private:
        NewtonProcessor *newtonProcessor;

        Entity *entity;
        NewtonBody *newtonBody;

    public:
        RigidNewtonBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Entity *entity)
            : newtonProcessor(static_cast<NewtonProcessor *>(NewtonWorldGetUserData(newtonWorld)))
            , entity(entity)
            , newtonBody(nullptr)
        {
            GEK_REQUIRE(newtonProcessor);
            GEK_REQUIRE(entity);

            auto &mass = entity->getComponent<MassComponent>();
            auto &transform = entity->getComponent<TransformComponent>();
            Math::Float4x4 matrix = transform.getMatrix();

            newtonBody = NewtonCreateDynamicBody(newtonWorld, newtonCollision, matrix.data);
            NewtonBodySetMassProperties(newtonBody, mass.value, newtonCollision);
            NewtonBodySetMatrix(newtonBody, matrix.data);
            NewtonBodySetUserData(newtonBody, dynamic_cast<NewtonEntity *>(this));
        }

        ~RigidNewtonBody(void)
        {
            NewtonDestroyBody(newtonBody);
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

        void onPreUpdate(int threadHandle)
        {
            auto &mass = entity->getComponent<MassComponent>();
            auto &transform = entity->getComponent<TransformComponent>();

            NewtonCollisionSetScale(NewtonBodyGetCollision(newtonBody), transform.scale.x, transform.scale.y, transform.scale.z);

            Math::Float3 gravity(newtonProcessor->getGravity(transform.position));
            NewtonBodyAddForce(newtonBody, (gravity * mass.value).data);
        }

        void onSetTransform(const float* const matrixData, int threadHandle)
        {
            auto &transform = entity->getComponent<TransformComponent>();

            Math::Float4x4 matrix(matrixData);
            transform.position = matrix.translation;
            transform.rotation = matrix.getQuaternion();
        }
    };

    NewtonEntityPtr createRigidBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Entity *entity)
    {
        return makeShared<NewtonEntity, RigidNewtonBody>(newtonWorld, newtonCollision, entity);
    }

    RigidBodyComponent::RigidBodyComponent(void)
    {
    }

    void RigidBodyComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, shape);
        saveParameter(componentData, L"surface", surface);
    }

    void RigidBodyComponent::load(const Population::ComponentDefinition &componentData)
    {
        shape = loadParameter<String>(componentData, nullptr);
        surface = loadParameter<String>(componentData, L"surface");
    }

    class RigidBodyImplementation
        : public ContextRegistration<RigidBodyImplementation>
        , public ComponentMixin<RigidBodyComponent>
    {
    public:
        RigidBodyImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"rigid_body";
        }
    };

    GEK_REGISTER_CONTEXT_USER(RigidBodyImplementation)
}; // namespace Gek