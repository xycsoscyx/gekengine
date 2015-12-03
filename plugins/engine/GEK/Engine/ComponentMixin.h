#pragma once

#include "GEK\Engine\Population.h"
#include "GEK\Engine\Component.h"

namespace Gek
{
    template <typename TYPE, typename CONVERTER>
    void setParameter(const std::unordered_map<CStringW, CStringW> &list, LPCWSTR name, TYPE &value, CONVERTER convert)
    {
        auto iterator = list.find(name);
        if (iterator != list.end())
        {
            value = convert((*iterator).second);
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