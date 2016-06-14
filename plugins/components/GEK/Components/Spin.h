#pragma once

#include "GEK\Math\Float3.h"
#include "GEK\Engine\Component.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct SpinComponent
    {
        SpinComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
