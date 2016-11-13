#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Math\Common.hpp"
#include "GEK\Math\SIMD\Matrix4x4.hpp"
#include <random>

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Spin)
        {
            Math::Float3 torque;

            void save(JSON::Object &componentData) const
            {
            }

            void load(const JSON::Object &componentData)
            {
                torque = Evaluator::Get<Math::Float3>(L"(random(-pi,pi), random(-pi,pi), random(-pi,pi))");
            }
        };
    }; // namespace Components

    GEK_CONTEXT_USER(Spin)
        , public Plugin::ComponentMixin<Components::Spin, Edit::Component>
    {
    public:
        Spin(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
        }

        void edit(ImGuiContext *guiContext, const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
        }
    };

    GEK_CONTEXT_USER(SpinProcessor, Plugin::Core *)
        , public Plugin::Processor
    {
    private:
        Plugin::Population *population;

    public:
        SpinProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
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

            population->listEntities<Components::Transform, Components::Spin>([&](Plugin::Entity *entity, const wchar_t *, auto &transformComponent, auto &spinComponent) -> void
            {
                Math::SIMD::Quaternion rotation(Math::SIMD::Quaternion::FromEuler((population->getFrameTime() * spinComponent.torque.x),
                    (population->getFrameTime() * spinComponent.torque.y),
                    (population->getFrameTime() * spinComponent.torque.z)));
                transformComponent.rotation *= rotation;
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(Spin);
    GEK_REGISTER_CONTEXT_USER(SpinProcessor);
}; // namespace Gek
