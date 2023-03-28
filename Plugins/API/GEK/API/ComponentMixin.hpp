/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: fc6dba5a2aba4d25dcd872ccbb719e3619e40901 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Wed Oct 19 17:38:40 2016 +0000 $
#pragma once

#include "GEK/API/Component.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/Population.hpp"
#include <concurrent_unordered_map.h>
#include <new>
#include <imgui.h>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        template <class COMPONENT, class BASE = Plugin::Component>
        class ComponentMixin
            : public BASE
        {
        protected:
            Population *population = nullptr;

        public:
            ComponentMixin(Population *population)
                : population(population)
            {
            }

            virtual ~ComponentMixin(void) = default;

            // Plugin::Component
			std::string_view getName(void) const
            {
				return COMPONENT::GetName();
            }

			Hash getIdentifier(void) const
            {
				return COMPONENT::GetIdentifier();
            }

            std::unique_ptr<Plugin::Component::Data> create(void)
            {
                return std::make_unique<COMPONENT>();
            }

            template <typename TYPE>
            TYPE evaluate(JSON const &object, TYPE defaultValue)
            {
                return object.evaluate(population->getShuntingYard(), defaultValue);
            }

            virtual void save(COMPONENT const * const component, JSON &exportData) const { };
            virtual void load(COMPONENT * const component, JSON const &importData) { };

            void save(Plugin::Component::Data const * const component, JSON &exportData) const
            {
                save(static_cast<COMPONENT const * const>(component), exportData);
            }

            void load(Plugin::Component::Data * const component, JSON const &importData)
            {
                load(static_cast<COMPONENT * const>(component), importData);
            }

            bool editorElement(std::string_view const &text, std::function<bool(void)> &&element)
            {
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(&*std::begin(text), &*std::end(text));
                ImGui::SameLine();
                ImGui::PushItemWidth(-1.0f);
                bool changed = element();
                ImGui::PopItemWidth();
                return changed;
            }
        };

        template <class CLASS, typename... REQUIRED>
		class EntityProcessor
			: public Plugin::Processor
        {
        private:
            struct Data : public CLASS::Data
            {
            };

        protected:
            using EntityDataMap = concurrency::concurrent_unordered_map<Plugin::Entity *, Data>;
            EntityDataMap entityDataMap;

        public:
            virtual ~EntityProcessor(void) = default;

            // ProcessorMixin
            void clear(void)
            {
                entityDataMap.clear();
            }

            void addEntity(Plugin::Entity * const entity, std::function<void(bool isNewInsert, Data &data, REQUIRED&... components)> onAdded = nullptr)
            {
                assert(entity);

                if (entity->hasComponents<REQUIRED...>())
                {
                    static const Data BlankData;
                    auto insertSearch = entityDataMap.insert(std::make_pair(entity, BlankData));
                    if (onAdded)
                    {
                        onAdded(insertSearch.second, insertSearch.first->second, entity->getComponent<REQUIRED>()...);
                    }
                }
            }

            void removeEntity(Plugin::Entity * const entity)
            {
                assert(entity);

                auto entitySearch = entityDataMap.find(entity);
                if (entitySearch != std::end(entityDataMap))
                {
                    entityDataMap.unsafe_erase(entitySearch);
                }
            }

            size_t getEntityCount(void)
            {
                return entityDataMap.size();
            }

            void listEntities(std::function<void(Plugin::Entity * const entity, Data &data, REQUIRED&... components)> &&onEntity)
            {
                assert(onEntity);

                std::for_each(std::begin(entityDataMap), std::end(entityDataMap), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }

            void parallelListEntities(std::function<void(Plugin::Entity * const entity, Data &data, REQUIRED&... components)> &&onEntity)
            {
                assert(onEntity);

                concurrency::parallel_for_each(std::begin(entityDataMap), std::end(entityDataMap), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }
        };
    }; // namespace Plugin
}; // namespace Gek