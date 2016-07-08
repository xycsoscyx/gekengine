#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Material)
        {
            GEK_START_EXCEPTIONS();

            using ResourceMap = std::unordered_map<String, ResourceHandle>;
            using PassMap = std::unordered_map<String, ResourceMap>;
        };
    }; // namespace Engine
}; // namespace Gek
