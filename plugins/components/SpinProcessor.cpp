#include "GEK\Context\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Math\Common.hpp"
#include "GEK\Math\Float4x4.hpp"
#include <random>

namespace Gek
{
    namespace Components
    {
        static std::random_device randomDevice;
        static std::mt19937 mersineTwister(randomDevice());
        static std::uniform_real_distribution<float> random(-1.0f, 1.0f);

        GEK_COMPONENT(Spin)
        {
            Math::Float3 torque;

            void save(Xml::Leaf &componentData) const
            {
            }

            void load(const Xml::Leaf &componentData)
            {
                torque.x = random(mersineTwister);
                torque.y = random(mersineTwister);
                torque.z = random(mersineTwister);
            }
        };
    }; // namespace Components

    GEK_CONTEXT_USER(Spin)
        , public Plugin::ComponentMixin<Components::Spin, Editor::Component>
    {
    public:
        Spin(Context *context)
            : ContextRegistration(context)
        {
        }

        // Editor::Component
        void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &spinComponent = *dynamic_cast<Components::Spin *>(data);
            ImGui::SetCurrentContext(nullptr);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"spin";
        }
    };

    GEK_CONTEXT_USER(SpinProcessor, Plugin::Core *)
        , public Plugin::PopulationStep
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

            population->addStep(this, 0);
        }

        ~SpinProcessor(void)
        {
            population->removeStep(this);
        }

        // Plugin::PopulationStep
        void onUpdate(uint32_t order, State state)
        {
            GEK_REQUIRE(population);

            if (state == State::Active)
            {
                population->listEntities<Components::Transform, Components::Spin>([&](Plugin::Entity *entity, const wchar_t *) -> void
                {
                    auto &transform = entity->getComponent<Components::Transform>();
                    auto &spin = entity->getComponent<Components::Spin>();
                    Math::Quaternion rotation(Math::Quaternion::createEulerRotation((population->getFrameTime() * spin.torque.x),
                                                                                    (population->getFrameTime() * spin.torque.y),
                                                                                    (population->getFrameTime() * spin.torque.z)));
                    transform.rotation *= rotation;
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Spin);
    GEK_REGISTER_CONTEXT_USER(SpinProcessor);
}; // namespace Gek
