#pragma once

#include "GEK\Utility\Evaluator.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Component.h"

namespace Gek
{
    template <typename TYPE>
    void saveParameter(Population::ComponentDefinition &componentData, LPCWSTR name, const TYPE &value)
    {
        if (name)
        {
            componentData[name] = String::from<wchar_t>(value);
        }
        else
        {
            componentData.SetString(String::from<wchar_t>(value));
        }
    }

    template <typename TYPE>
    bool loadParameter(const Population::ComponentDefinition &componentData, LPCWSTR name, TYPE &value)
    {
        if (name)
        {
            auto iterator = componentData.find(name);
            if (iterator != componentData.end())
            {
                value = Evaluator::get<TYPE>((*iterator).second);
                //value = String::to<TYPE>((*iterator).second);
                return true;
            }
        }
        else if(!componentData.IsEmpty())
        {
            value = Evaluator::get<TYPE>(componentData.GetString());
        }

        return false;
    }

    template <class DATA>
    class ComponentMixin : public Component
    {
    public:
        ComponentMixin(void)
        {
        }

        virtual ~ComponentMixin(void)
        {
        }

        // Component
        STDMETHODIMP_(std::type_index) getIdentifier(void) const
        {
            return typeid(DATA);
        }

        STDMETHODIMP_(LPVOID) create(const Population::ComponentDefinition &componentData)
        {
            DATA *data = new DATA();
            if (data)
            {
                data->load(componentData);
            }

            return data;
        }

        STDMETHODIMP_(void) destroy(LPVOID voidData)
        {
            DATA *data = static_cast<DATA *>(voidData);
            delete data;
        }
    };
}; // namespace Gek