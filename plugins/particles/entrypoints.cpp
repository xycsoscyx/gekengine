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

GEK_PLUGIN_BEGIN(Components)
    GEK_PLUGIN_CLASS(Gek::ParticlesRegistration, Gek::ParticlesImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::ParticlesProcessorRegistration, Gek::ParticlesProcessorImplementation)
        GEK_PLUGIN_TYPE(Gek::ProcessorType)
GEK_PLUGIN_END
