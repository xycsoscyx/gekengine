#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Core.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/Components/Name.hpp"

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
        Edit::Events *events = nullptr;

        using NameMap = std::unordered_map<std::string, Plugin::Entity *>;
        NameMap nameMap;

    public:
        NameProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , population(core->getPopulation())
        {
            assert(population);

            core->onInitialized.connect(this, &NameProcessor::onInitialized);
            core->onShutdown.connect(this, &NameProcessor::onShutdown);
            population->onReset.connect(this, &NameProcessor::onReset);
            population->onEntityCreated.connect(this, &NameProcessor::onEntityCreated);
            population->onEntityDestroyed.connect(this, &NameProcessor::onEntityDestroyed);
            population->onComponentAdded.connect(this, &NameProcessor::onComponentAdded);
            population->onComponentRemoved.connect(this, &NameProcessor::onComponentRemoved);
        }

        uint32_t uniqueIdentifier = 0;
        void addEntity(Plugin::Entity * const entity)
        {
            ProcessorMixin::addEntity(entity, [&](bool isNewInsert, auto &data, auto &nameComponent) -> void
            {
                auto nameSearch = nameMap.find(nameComponent.name);
                if (nameSearch != std::end(nameMap) && nameSearch->second != entity)
                {
                    nameComponent.name += String::Format("{}", ++uniqueIdentifier);
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

        // Plugin::Core
        void onInitialized(void)
        {
            core->listProcessors([&](Plugin::Processor *processor) -> void
            {
                auto castCheck = dynamic_cast<Edit::Events *>(processor);
                if (castCheck)
                {
                    (events = castCheck)->onModified.connect(this, &NameProcessor::onModified);
                }
            });
        }

        void onShutdown(void)
        {
            if (events)
            {
                events->onModified.disconnect(this, &NameProcessor::onModified);
            }

            population->onReset.disconnect(this, &NameProcessor::onReset);
            population->onEntityCreated.disconnect(this, &NameProcessor::onEntityCreated);
            population->onEntityDestroyed.disconnect(this, &NameProcessor::onEntityDestroyed);
            population->onComponentAdded.disconnect(this, &NameProcessor::onComponentAdded);
            population->onComponentRemoved.disconnect(this, &NameProcessor::onComponentRemoved);
        }

        // Model::Processor
        Plugin::Entity *getEntity(std::string const &name)
        {
            return nullptr;
        }

        // Plugin::Editor Slots
        void onModified(Plugin ::Entity * const entity, const std::type_index &type)
        {
            if (type == typeid(Components::Name))
            {
                addEntity(entity);
            }
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
