#pragma once

#include "GEK\Utility\Evaluator.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Component.h"
#include <new>

namespace Gek
{
    namespace Plugin
    {
        template <typename TYPE>
        void saveParameter(Plugin::Population::ComponentDefinition &componentData, const wchar_t *name, const TYPE &value)
        {
            if (name)
            {
                componentData[name] = value;
            }
            else
            {
                componentData.value = value;
            }
        }

        template <typename TYPE>
        TYPE loadParameter(const Plugin::Population::ComponentDefinition &componentData, const wchar_t *name, const TYPE &defaultValue)
        {
            if (name)
            {
                auto componentSearch = componentData.find(name);
                if (componentSearch != componentData.end())
                {
                    return Evaluator::get<TYPE>(componentSearch->second);
                }
            }
            else if (!componentData.value.empty())
            {
                return Evaluator::get<TYPE>(componentData.value);
            }

            return defaultValue;
        }

        template <class DATA>
        class ComponentMixin
            : public Component
        {
        public:
            ComponentMixin(void)
            {
            }

            virtual ~ComponentMixin(void)
            {
            }

            // Plugin::Component
            std::type_index getIdentifier(void) const
            {
                return typeid(DATA);
            }

            std::unique_ptr<Data> create(const Plugin::Population::ComponentDefinition &componentData)
            {
                auto data = std::make_unique<DATA>();
                data->load(componentData);
                return data;
            }
        };
    }; // namespace Plugin
}; // namespace Gek