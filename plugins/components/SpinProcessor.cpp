#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Math\Common.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
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

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"spin";
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
                Math::QuaternionFloat rotation(Math::QuaternionFloat::createEulerRotation((population->getFrameTime() * spinComponent.torque.x),
                    (population->getFrameTime() * spinComponent.torque.y),
                    (population->getFrameTime() * spinComponent.torque.z)));
                transformComponent.rotation *= rotation;
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(Spin);
    GEK_REGISTER_CONTEXT_USER(SpinProcessor);
}; // namespace Gek
