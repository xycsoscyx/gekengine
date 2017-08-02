#include "GEK/Utility/ContextUser.hpp"
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

        using NameMap = std::unordered_map<std::string, Plugin::Entity *>;
        NameMap nameMap;

    public:
        NameProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , population(core->getPopulation())
        {
            assert(population);

            population->onReset.connect(this, &NameProcessor::onReset, this);
            population->onEntityCreated.connect(this, &NameProcessor::onEntityCreated, this);
            population->onEntityDestroyed.connect(this, &NameProcessor::onEntityDestroyed, this);
            population->onComponentAdded.connect(this, &NameProcessor::onComponentAdded, this);
            population->onComponentRemoved.connect(this, &NameProcessor::onComponentRemoved, this);
        }

        ~NameProcessor(void)
        {
        }

        uint32_t uniqueIdentifier = 0;
        void addEntity(Plugin::Entity * const entity)
        {
            ProcessorMixin::addEntity(entity, [&](bool isNewInsert, auto &data, auto &nameComponent) -> void
            {
                auto nameSearch = nameMap.find(nameComponent.name);
                if (nameSearch != std::end(nameMap))
                {
                    nameComponent.name += String::Format("%v", ++uniqueIdentifier);
                }

                nameMap.insert(std::make_pair(nameComponent.name, entity));
            });
        }

        void removeEntity(Plugin::Entity * const entity)
        {
            if (entity->hasComponent<Components::Name>())
            {
                auto &nameComponent = entity->getComponent<Components::Name>();
                if (nameComponent.name.empty())
                {
                    auto nameSearch = std::find_if(std::begin(nameMap), std::end(nameMap), [entity](NameMap::value_type &valuePair) -> bool
                    {
                        return (valuePair.second == entity);
                    });

                    if (nameSearch != std::end(nameMap))
                    {
                        nameMap.erase(nameSearch);
                    }
                }
                else
                {
                    auto nameSearch = nameMap.find(nameComponent.name);
                    if (nameSearch != std::end(nameMap))
                    {
                        nameMap.erase(nameSearch);
                    }
                }

                ProcessorMixin::removeEntity(entity);
            }
        }

        // Plugin::Processor
        void onInitialized(void)
        {
            core->listProcessors([&](Plugin::Processor *processor) -> void
            {
                auto castCheck = dynamic_cast<Plugin::Editor *>(processor);
                if (castCheck)
                {
                    (editor = castCheck)->onModified.connect(this, &NameProcessor::onModified);
                }
            });
        }

        void onDestroyed(void)
        {
            if (editor)
            {
                editor->onModified.disconnect(this);
            }

            population->onReset.disconnect(this);
            population->onEntityCreated.disconnect(this);
            population->onEntityDestroyed.disconnect(this);
            population->onComponentAdded.disconnect(this);
            population->onComponentRemoved.disconnect(this);
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
        void onReset(void)
        {
            clear();
        }

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
