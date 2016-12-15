/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Hash.hpp"

namespace Gek
{
    template <typename TYPE, uint8_t UNIQUE>
    struct Handle
    {
        TYPE identifier = 0;

        Handle(uint32_t identifier = 0)
            : identifier(TYPE(identifier))
        {
        }

        operator bool() const
        {
            return (identifier != 0);
        }

        operator std::size_t() const
        {
            return identifier;
        }

        bool operator == (const typename Handle<TYPE, UNIQUE> &handle) const
        {
            return (identifier == handle.identifier);
        }

        bool operator != (const typename Handle<TYPE, UNIQUE> &handle) const
        {
            return (identifier != handle.identifier);
        }
    };

    using RenderStateHandle = Handle<uint8_t, __LINE__>;
    using DepthStateHandle = Handle<uint8_t, __LINE__>;
    using BlendStateHandle = Handle<uint8_t, __LINE__>;
    using ProgramHandle = Handle<uint16_t, __LINE__>;
    using VisualHandle = Handle<uint8_t, __LINE__>;
    using ShaderHandle = Handle<uint8_t, __LINE__>;
    using MaterialHandle = Handle<uint16_t, __LINE__>;
    using ResourceHandle = Handle<uint32_t, __LINE__>;
}; // namespace Gek

namespace std
{
    template <typename TYPE, int UNIQUE>
    struct hash<Gek::Handle<TYPE, UNIQUE>>
    {
        size_t operator()(const Gek::Handle<TYPE, UNIQUE> &value) const
        {
            return value.identifier;
        }
    };
};
