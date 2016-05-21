#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\COM.h"
#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(ParticlesImplementation);
    DECLARE_INTERFACE_IID(ParticlesRegistration, "31BE4D09-C2E4-454D-8215-31B8B3CA059D");

    DECLARE_REGISTERED_CLASS(ParticlesProcessorImplementation);
    DECLARE_INTERFACE_IID(ParticlesProcessorRegistration, "795F8115-A582-4BEA-ABED-32A2BC1764F7");
}; // namespace Gek

DECLARE_PLUGIN_MAP(Components)
    ADD_PLUGIN_CLASS(Gek::ParticlesRegistration, Gek::ParticlesImplementation)
        ADD_PLUGIN_CLASS_TYPE(Gek::ComponentType)

    ADD_PLUGIN_CLASS(Gek::ParticlesProcessorRegistration, Gek::ParticlesProcessorImplementation)
        ADD_PLUGIN_CLASS_TYPE(Gek::ProcessorType)
END_PLUGIN_MAP
