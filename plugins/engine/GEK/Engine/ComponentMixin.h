#pragma once

#include "GEK\Utility\Evaluator.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Component.h"

namespace Gek
{
    template <typename TYPE>
    void saveParameter(Population::ComponentDefinition &componentData, const wchar_t *name, const TYPE &value)
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
    TYPE loadParameter(const Population::ComponentDefinition &componentData, const wchar_t *name, const TYPE &defaultValue = TYPE())
    {
        if (name)
        {
            auto iterator = componentData.find(name);
            if (iterator != componentData.end())
            {
                return Evaluator::get<TYPE>((*iterator).second);
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

        // Component
        std::type_index getIdentifier(void) const
        {
            return typeid(DATA);
        }

        void *createData(const Population::ComponentDefinition &componentData)
        {
            DATA *data = new DATA();
            GEK_CHECK_CONDITION(data == nullptr, Trace::Exception, "Unable to create component data: %v", typeid(DATA).name());

            data->load(componentData);

            return data;
        }

        void destroyData(void *data)
        {
            GEK_REQUIRE(data);
            delete static_cast<DATA *>(data);
        }
    };
}; // namespace Gek