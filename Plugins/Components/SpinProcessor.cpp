#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Core.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/API/Entity.hpp"
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
			GEK_COMPONENT_DATA(Spin);

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
		void save(Components::Spin const * const data, JSON::Object &exportData) const
		{
		}

		void load(Components::Spin * const data, JSON::Object const &importData)
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

			core->onShutdown.connect(this, &SpinProcessor::onShutdown);
			population->onUpdate[50].connect(this, &SpinProcessor::onUpdate);
		}

		// Plugin::Core
		void onShutdown(void)
		{
			population->onUpdate[50].disconnect(this, &SpinProcessor::onUpdate);
		}

		// Plugin::Population Slots
		void onUpdate(float frameTime)
		{
			assert(population);

			bool editorActive = core->getOption("editor", "active", false);
			if (frameTime > 0.0f && !editorActive)
			{
				auto onEntity = [&](Plugin::Entity * const entity) -> void
				{
                    if (entity->hasComponents<Components::Transform, Components::Spin>())
                    {
						auto &spinComponent = entity->getComponent<Components::Spin>();
						auto omega(spinComponent.torque * frameTime);

						auto &transformComponent = entity->getComponent<Components::Transform>();
						transformComponent.rotation *= Math::Quaternion::MakeEulerRotation(omega.x, omega.y, omega.z);
                    }
				};

				population->listEntities(onEntity);
			}
		}
	};

	GEK_REGISTER_CONTEXT_USER(Spin);
	GEK_REGISTER_CONTEXT_USER(SpinProcessor);
}; // namespace Gek
