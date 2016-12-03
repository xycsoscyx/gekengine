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
        void save(const Components::Spin *data, JSON::Object &componentData) const
        {
        }

        void load(Components::Spin *data, const JSON::Object &componentData)
        {
            data->torque = Evaluator::Get<Math::Float3>(population->getShuntingYard(), L"(random(-pi,pi), random(-pi,pi), random(-pi,pi))");
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
            GEK_REQUIRE(population);

            population->onUpdate[50].connect<SpinProcessor, &SpinProcessor::onUpdate>(this);
        }

        ~SpinProcessor(void)
        {
            population->onUpdate[50].disconnect<SpinProcessor, &SpinProcessor::onUpdate>(this);
        }

        // Plugin::Population Slots
        void onUpdate(void)
        {
            GEK_REQUIRE(population);

            if (!core->isEditorActive())
            {
                population->listEntities<Components::Transform, Components::Spin>([&](Plugin::Entity *entity, const wchar_t *, auto &transformComponent, auto &spinComponent) -> void
                {
                    auto omega(spinComponent.torque * population->getFrameTime());
                    transformComponent.rotation *= Math::Quaternion::FromEuler(omega.x, omega.y, omega.z);
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Spin);
    GEK_REGISTER_CONTEXT_USER(SpinProcessor);
}; // namespace Gek
