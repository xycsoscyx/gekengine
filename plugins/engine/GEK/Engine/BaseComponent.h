#pragma once

#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"

namespace Gek
{
    namespace Engine
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
        class BaseComponent : public Component::Interface
        {
        public:
            BaseComponent(void)
            {
            }

            virtual ~BaseComponent(void)
            {
            }

            // Component::Interface
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
    }; // namespace Engine
}; // namespace Gek