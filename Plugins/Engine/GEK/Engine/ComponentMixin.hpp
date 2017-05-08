/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: fc6dba5a2aba4d25dcd872ccbb719e3619e40901 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Wed Oct 19 17:38:40 2016 +0000 $
#pragma once

#include "GEK/Engine/Component.hpp"
#include "GEK/Engine/Population.hpp"
#include <concurrent_unordered_map.h>
#include <new>

namespace Gek
{
    namespace Plugin
    {
        GEK_PREDECLARE(Entity);

        template <class COMPONENT, class BASE = Plugin::Component>
        class ComponentMixin
            : public BASE
        {
        private:
            WString name;

        protected:
            Population *population = nullptr;

        public:
            ComponentMixin(Population *population)
                : population(population)
            {
                name = typeid(COMPONENT).name();
                auto colonPosition = name.rfind(':');
                if (colonPosition != CString::npos)
                {
                    name = name.subString(colonPosition + 1);
                }
            }

            virtual ~ComponentMixin(void) = default;

            // Plugin::Component
            wchar_t const * const getName(void) const
            {
                return name;
            }

            std::type_index getIdentifier(void) const
            {
                return typeid(COMPONENT);
            }

            std::unique_ptr<Plugin::Component::Data> create(void)
            {
                return std::make_unique<COMPONENT>();
            }

            bool getValue(JSON::Object const &data, bool defaultValue)
            {
				switch (data.type_id())
				{
				case jsoncons::value_type::small_string_t:
				case jsoncons::value_type::string_t:
					return population->getShuntingYard().evaluate(data.as_cstring(), defaultValue) != 0.0f;

				case jsoncons::value_type::bool_t:
					return data.var_.bool_data_cast()->value();

				case jsoncons::value_type::double_t:
					return data.var_.double_data_cast()->value() != 0.0;

				case jsoncons::value_type::integer_t:
					return data.var_.integer_data_cast()->value() != 0;

				case jsoncons::value_type::uinteger_t:
					return data.var_.uinteger_data_cast()->value() != 0;

				default:
					return defaultValue;
				};
			}

            int32_t getValue(JSON::Object const &data, int32_t defaultValue)
            {
				switch (data.type_id())
				{
				case jsoncons::value_type::small_string_t:
				case jsoncons::value_type::string_t:
					return static_cast<int32_t>(population->getShuntingYard().evaluate(data.as_cstring(), defaultValue));

				case jsoncons::value_type::double_t:
					return static_cast<int64_t>(data.var_.double_data_cast()->value());

				case jsoncons::value_type::integer_t:
					return static_cast<int64_t>(data.var_.integer_data_cast()->value());

				case jsoncons::value_type::uinteger_t:
					return static_cast<int64_t>(data.var_.uinteger_data_cast()->value());

				case jsoncons::value_type::bool_t:
					return data.var_.bool_data_cast()->value() ? 1 : 0;

				default:
					return defaultValue;
				};
			}

            uint32_t getValue(JSON::Object const &data, uint32_t defaultValue)
            {
				switch (data.type_id())
				{
				case jsoncons::value_type::small_string_t:
				case jsoncons::value_type::string_t:
					return static_cast<uint32_t>(population->getShuntingYard().evaluate(data.as_cstring(), defaultValue));

				case jsoncons::value_type::double_t:
					return static_cast<uint32_t>(data.var_.double_data_cast()->value());

				case jsoncons::value_type::integer_t:
					return static_cast<uint32_t>(data.var_.integer_data_cast()->value());

				case jsoncons::value_type::uinteger_t:
					return static_cast<uint32_t>(data.var_.uinteger_data_cast()->value());

				case jsoncons::value_type::bool_t:
					return data.var_.bool_data_cast()->value() ? 1 : 0;

				default:
					return defaultValue;
				};
			}

            float getValue(JSON::Object const &data, float defaultValue)
            {
				switch (data.type_id())
				{
				case jsoncons::value_type::small_string_t:
				case jsoncons::value_type::string_t:
					return population->getShuntingYard().evaluate(data.as_cstring(), defaultValue);

				case jsoncons::value_type::double_t:
					return static_cast<float>(data.var_.double_data_cast()->value());

				case jsoncons::value_type::integer_t:
					return static_cast<float>(data.var_.integer_data_cast()->value());

				case jsoncons::value_type::uinteger_t:
					return static_cast<float>(data.var_.uinteger_data_cast()->value());

				case jsoncons::value_type::bool_t:
					return data.var_.bool_data_cast()->value() ? 1.0f : 0.0f;

				default:
					return defaultValue;
				};
			}

            template <typename TYPE>
            Math::Vector2<TYPE> getValue(JSON::Object const &data, Math::Vector2<TYPE> const &defaultValue)
            {
                if (data.is_array() && data.size() == 2)
                {
                    return Math::Vector2<TYPE>(
                        getValue(data.at(0), defaultValue.x),
                        getValue(data.at(1), defaultValue.y));
                }

                return defaultValue;
            }

            template <typename TYPE>
            Math::Vector3<TYPE> getValue(JSON::Object const &data, Math::Vector3<TYPE> const &defaultValue)
            {
                if (data.is_array() && data.size() == 3)
                {
                    return Math::Vector3<TYPE>(
                        getValue(data.at(0), defaultValue.x),
                        getValue(data.at(1), defaultValue.y),
                        getValue(data.at(2), defaultValue.z));
                }

                return defaultValue;
            }

            template <typename TYPE>
            Math::Vector4<TYPE> getValue(JSON::Object const &data, Math::Vector4<TYPE> const &defaultValue)
            {
				if (data.is_array())
				{
					if (data.size() == 3)
					{
						return Math::Vector4<TYPE>(
							getValue(data.at(0), defaultValue.x),
							getValue(data.at(1), defaultValue.y),
							getValue(data.at(2), defaultValue.z), 1.0f);
					}
					else if (data.size() == 4)
					{
						return Math::Vector4<TYPE>(
							getValue(data.at(0), defaultValue.x),
							getValue(data.at(1), defaultValue.y),
							getValue(data.at(2), defaultValue.z),
							getValue(data.at(3), defaultValue.w));
					}
				}

                return defaultValue;
            }

            Math::Quaternion getValue(JSON::Object const &data, Math::Quaternion const &defaultValue)
            {
                if (data.is_array())
                {
                    if (data.size() == 3)
                    {
                        return Math::Quaternion::FromEuler(
                            getValue(data.at(0), defaultValue.x),
                            getValue(data.at(1), defaultValue.y),
                            getValue(data.at(2), defaultValue.z));
                    }
                    else if (data.size() == 4)
                    {
                        return Math::Quaternion(
                            getValue(data.at(0), defaultValue.x),
                            getValue(data.at(1), defaultValue.y),
                            getValue(data.at(2), defaultValue.z),
                            getValue(data.at(3), defaultValue.w));
                    }
                }

                return defaultValue;
            }

            WString getValue(JSON::Object const &data, WString const &defaultValue)
            {
                return data.as_string();
            }

            template <typename TYPE>
            TYPE getValue(JSON::Object const &componentData, WString const &name, TYPE const &defaultValue)
            {
                if (componentData.is_object() && componentData.has_member(name))
                {
                    auto &data = componentData.get(name);
                    return getValue(data, defaultValue);
                }

                return defaultValue;
            }

            virtual void save(COMPONENT const * const component, JSON::Object &componentData) const { };
            virtual void load(COMPONENT * const component, JSON::Object const &componentData) { };

            void save(Plugin::Component::Data const * const component, JSON::Object &componentData) const
            {
                save(static_cast<COMPONENT const * const>(component), componentData);
            }

            void load(Plugin::Component::Data * const component, JSON::Object const &componentData)
            {
                load(static_cast<COMPONENT * const>(component), componentData);
            }
        };

        template <class CLASS, typename... REQUIRED>
        class ProcessorMixin
        {
        private:
            struct Data : public CLASS::Data
            {
            };

        protected:
            using EntityDataMap = concurrency::concurrent_unordered_map<Plugin::Entity *, Data>;
            EntityDataMap entityDataMap;

        public:
            virtual ~ProcessorMixin(void) = default;

            // ProcessorMixin
            void clear(void)
            {
                entityDataMap.clear();
            }

            void addEntity(Plugin::Entity * const entity, std::function<void(Data &data, REQUIRED&... components)> onAdded)
            {
                GEK_REQUIRE(entity);

                if (entity->hasComponents<REQUIRED...>())
                {
                    auto insertSearch = entityDataMap.insert(std::make_pair(entity, Data()));
                    if (insertSearch.second && onAdded)
                    {
                        onAdded(insertSearch.first->second, entity->getComponent<REQUIRED>()...);
                    }
                }
            }

            void removeEntity(Plugin::Entity * const entity)
            {
                GEK_REQUIRE(entity);

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

            void listEntities(std::function<void(Plugin::Entity * const entity, Data &data, REQUIRED&... components)> onEntity)
            {
                GEK_REQUIRE(onEntity);

                std::for_each(std::begin(entityDataMap), std::end(entityDataMap), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }

            void parallelListEntities(std::function<void(Plugin::Entity * const entity, Data &data, REQUIRED&... components)> onEntity)
            {
                GEK_REQUIRE(onEntity);

                concurrency::parallel_for_each(std::begin(entityDataMap), std::end(entityDataMap), [&](auto &entitySearch) -> void
                {
                    onEntity(entitySearch.first, entitySearch.second, entitySearch.first->getComponent<REQUIRED>()...);
                });
            }
        };
    }; // namespace Plugin
}; // namespace Gek