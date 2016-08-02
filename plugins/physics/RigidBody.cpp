#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\World.h"
#include "GEK\Newton\Entity.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Newton\RigidBody.h"
#include <Newton.h>

namespace Gek
{
    class RigidNewtonBody
        : public Newton::Entity
    {
    private:
        Newton::World *world;

        Plugin::Entity *entity;
        NewtonBody *newtonBody;

    public:
        RigidNewtonBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Plugin::Entity *entity)
            : world(static_cast<Newton::World *>(NewtonWorldGetUserData(newtonWorld)))
            , entity(entity)
            , newtonBody(nullptr)
        {
            GEK_REQUIRE(world);
            GEK_REQUIRE(entity);

            auto &mass = entity->getComponent<Components::Mass>();
            auto &transform = entity->getComponent<Components::Transform>();

            Math::Float4x4 matrix(transform.getMatrix());
            newtonBody = NewtonCreateDynamicBody(newtonWorld, newtonCollision, matrix.data);
            NewtonBodySetUserData(newtonBody, dynamic_cast<Newton::Entity *>(this));
            NewtonBodySetMassProperties(newtonBody, mass.value, newtonCollision);
        }

        ~RigidNewtonBody(void)
        {
            NewtonDestroyBody(newtonBody);
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

        void onPreUpdate(int threadHandle)
        {
            auto &mass = entity->getComponent<Components::Mass>();
            auto &transform = entity->getComponent<Components::Transform>();

            NewtonCollisionSetScale(NewtonBodyGetCollision(newtonBody), transform.scale.x, transform.scale.y, transform.scale.z);

            Math::Float3 gravity(world->getGravity(transform.position));
            NewtonBodyAddForce(newtonBody, (gravity * mass.value).data);
        }

        void onSetTransform(const float* const matrixData, int threadHandle)
        {
            auto &transform = entity->getComponent<Components::Transform>();

            Math::Float4x4 matrix(matrixData);
            transform.position = matrix.translation;
            transform.rotation = matrix.getQuaternion();
        }
    };

    Newton::EntityPtr createRigidBody(NewtonWorld *newtonWorld, const NewtonCollision* const newtonCollision, Plugin::Entity *entity)
    {
        return std::make_shared<RigidNewtonBody>(newtonWorld, newtonCollision, entity);
    }

    namespace Components
    {
        RigidBody::RigidBody(void)
        {
        }

        void RigidBody::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, nullptr, shape);
            saveParameter(componentData, L"surface", surface);
        }

        void RigidBody::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            shape = loadParameter(componentData, nullptr, String());
            surface = loadParameter(componentData, L"surface", String());
        }
    }; // namespace Components

    GEK_CONTEXT_USER(RigidBody)
        , public Plugin::ComponentMixin<Components::RigidBody>
    {
    public:
        RigidBody(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"rigid_body";
        }
    };

    GEK_REGISTER_CONTEXT_USER(RigidBody)
}; // namespace Gek