﻿#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Editor.hpp"
#include "GEK/Components/Name.hpp"
#include <concurrent_unordered_map.h>
#include <random>

namespace Gek
{
    GEK_CONTEXT_USER(Name, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Name, Edit::Component>
    {
    public:
        Name(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Name const * const data, JSON::Object &componentData) const
        {
            componentData = data->name;
        }

        void load(Components::Name * const data, JSON::Reference componentData)
        {
            data->name = componentData.convert(String::Empty);
        }

        // Edit::Component
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &nameComponent = *dynamic_cast<Components::Name *>(data);

            changed |= editorElement("Name", [&](void) -> bool
            {
                return UI::InputString("##name", nameComponent.name, ImGuiInputTextFlags_EnterReturnsTrue);
            });

            ImGui::SetCurrentContext(nullptr);
            return changed;
        }
    };

    GEK_CONTEXT_USER(NameProcessor, Plugin::Core *)
        , public Plugin::ProcessorMixin<NameProcessor, Components::Name>
        , public Plugin::Processor
        , public Gek::Processor::Name
    {
    public:
        struct Data
        {
        };

    private:
        Plugin::Core *core = nullptr;
        Plugin::Population *population = nullptr;
        Plugin::Editor *editor = nullptr;

    public:
        NameProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , population(core->getPopulation())
        {
            assert(population);

            population->onEntityCreated.connect<NameProcessor, &NameProcessor::onEntityCreated>(this);
            population->onEntityDestroyed.connect<NameProcessor, &NameProcessor::onEntityDestroyed>(this);
            population->onComponentAdded.connect<NameProcessor, &NameProcessor::onComponentAdded>(this);
            population->onComponentRemoved.connect<NameProcessor, &NameProcessor::onComponentRemoved>(this);
        }

        ~NameProcessor(void)
        {
            population->onComponentRemoved.disconnect<NameProcessor, &NameProcessor::onComponentRemoved>(this);
            population->onComponentAdded.disconnect<NameProcessor, &NameProcessor::onComponentAdded>(this);
            population->onEntityDestroyed.disconnect<NameProcessor, &NameProcessor::onEntityDestroyed>(this);
            population->onEntityCreated.disconnect<NameProcessor, &NameProcessor::onEntityCreated>(this);
        }

        void addEntity(Plugin::Entity * const entity)
        {
            ProcessorMixin::addEntity(entity, [&](bool isNewInsert, auto &data, auto &nameComponent) -> void
            {
            });
        }

        void removeEntity(Plugin::Entity * const entity)
        {
            ProcessorMixin::removeEntity(entity);
        }

        // Plugin::Processor
        void onInitialized(void)
        {
            core->listProcessors([&](Plugin::Processor *processor) -> void
            {
                auto check = dynamic_cast<Plugin::Editor *>(processor);
                if (check)
                {
                    editor = check;
                    editor->onModified.connect<NameProcessor, &NameProcessor::onModified>(this);
                }
            });
        }

        void onDestroyed(void)
        {
            if (editor)
            {
                editor->onModified.disconnect<NameProcessor, &NameProcessor::onModified>(this);
            }
        }

        // Model::Processor
        Plugin::Entity *getEntity(std::string const &name)
        {
            return nullptr;
        }

        // Plugin::Editor Slots
        void onModified(Plugin::Entity * const entity, const std::type_index &type)
        {
            addEntity(entity);
        }

        // Plugin::Population Slots
        void onEntityCreated(Plugin::Entity * const entity)
        {
            addEntity(entity);
        }

        void onEntityDestroyed(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }

        void onComponentAdded(Plugin::Entity * const entity)
        {
            addEntity(entity);
        }

        void onComponentRemoved(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }
    };

    GEK_REGISTER_CONTEXT_USER(Name);
    GEK_REGISTER_CONTEXT_USER(NameProcessor);
}; // namespace Gek