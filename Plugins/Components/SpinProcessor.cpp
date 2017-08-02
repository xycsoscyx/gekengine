#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Processor.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include <random>

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Spin)
        {
            Math::Float3 torque;
        };
    }; // namespace Components

    GEK_CONTEXT_USER(Spin, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Spin>
    {
    public:
        Spin(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Spin const * const data, JSON::Object &componentData) const
        {
        }

        void load(Components::Spin * const data, JSON::Reference componentData)
        {
            data->torque.x = population->getShuntingYard().evaluate("random(-pi,pi)").value_or(0.0f);
            data->torque.y = population->getShuntingYard().evaluate("random(-pi,pi)").value_or(0.0f);
            data->torque.z = population->getShuntingYard().evaluate("random(-pi,pi)").value_or(0.0f);
        }
    };

    GEK_CONTEXT_USER(SpinProcessor, Plugin::Core *)
        , public Plugin::Processor
    {
    private:
        Plugin::Core *core = nullptr;
        Plugin::Population *population = nullptr;

    public:
        SpinProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , population(core->getPopulation())
        {
            assert(population);

            population->onUpdate[50].connect(this, &SpinProcessor::onUpdate);
        }

        ~SpinProcessor(void)
        {
        }

        // Plugin::Processor
        void onInitialized(void)
        {
        }

        void onDestroyed(void)
        {
            population->onUpdate[50].disconnect(this, &SpinProcessor::onUpdate);
        }

        // Plugin::Population Slots
        void onUpdate(float frameTime)
        {
            assert(population);

            bool editorActive = core->getOption("editor", "active").convert(false);
            if (frameTime > 0.0f && !editorActive)
            {
                population->listEntities<Components::Transform, Components::Spin>([&](Plugin::Entity * const entity, auto &transformComponent, auto &spinComponent) -> void
                {
                    auto omega(spinComponent.torque * frameTime);
                    transformComponent.rotation *= Math::Quaternion::MakeEulerRotation(omega.x, omega.y, omega.z);
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Spin);
    GEK_REGISTER_CONTEXT_USER(SpinProcessor);
}; // namespace Gek
