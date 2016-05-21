#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\COM.h"
#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(ModelImplementation);
    DECLARE_INTERFACE_IID(ModelRegistration, "9D68F36F-34C3-4E94-85F7-2420171BC5A9");

    DECLARE_REGISTERED_CLASS(ModelProcessorImplementation);
    DECLARE_INTERFACE_IID(ModelProcessorRegistration, "6AE951CC-3F57-45AF-A52A-29776E70CD22");
}; // namespace Gek

DECLARE_PLUGIN_MAP(Components)
    ADD_PLUGIN_CLASS(Gek::ModelRegistration, Gek::ModelImplementation)
        ADD_PLUGIN_CLASS_TYPE(Gek::ComponentType)

    ADD_PLUGIN_CLASS(Gek::ModelProcessorRegistration, Gek::ModelProcessorImplementation)
        ADD_PLUGIN_CLASS_TYPE(Gek::ProcessorType)
END_PLUGIN_MAP
