#pragma once

namespace Gek
{
    DECLARE_INTERFACE_IID(Entity, "F5F5D5D3-E409-437D-BF3B-801DFA230C6C") : virtual public IUnknown
    {
        STDMETHOD_(bool, hasComponent)              (THIS_ const std::type_index &type) PURE;
        STDMETHOD_(LPVOID, getComponent)            (THIS_ const std::type_index &type) PURE;

        template <typename CLASS>
        bool hasComponent(void)
        {
            return hasComponent(typeid(CLASS));
        }

        template<typename... ARGS>
        bool hasComponents(void)
        {
            std::vector<bool> hasComponents({ hasComponent<ARGS>()... });
            return std::find_if(hasComponents.begin(), hasComponents.end(), [&](bool hasComponent) -> bool
            {
                return !hasComponent;
            }) == hasComponents.end();
        }

        template <typename CLASS>
        CLASS &getComponent(void)
        {
            return *static_cast<CLASS *>(getComponent(typeid(CLASS)));
        }
    };
}; // namespace Gek
