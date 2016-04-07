#pragma once

#include "GEK\Engine\Population.h"
#include "GEK\Engine\Component.h"

namespace Gek
{
    template <typename TYPE>
    void saveParameter(std::unordered_map<CStringW, CStringW> &list, LPCWSTR name, const TYPE &value)
    {
        list[name] = String::from<wchar_t>(value);
    }

    template <typename TYPE>
    bool loadParameter(const std::unordered_map<CStringW, CStringW> &list, LPCWSTR name, TYPE &value)
    {
        auto iterator = list.find(name);
        if (iterator == list.end())
        {
            return false;
        }
        else
        {
            value = String::to<TYPE>((*iterator).second);
            return true;
        }
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

        STDMETHODIMP_(LPVOID) create(const std::unordered_map<CStringW, CStringW> &componentParameterList)
        {
            DATA *data = new DATA();
            if (data)
            {
                data->load(componentParameterList);
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