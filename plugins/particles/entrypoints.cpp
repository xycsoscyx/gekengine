#include <initguid.h>
#include <cguid.h>

#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

GEK_PLUGIN_BEGIN(Components)
    GEK_PLUGIN_CLASS(Gek::ParticlesRegistration, Gek::ParticlesImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::ParticlesProcessorRegistration, Gek::ParticlesProcessorImplementation)
        GEK_PLUGIN_TYPE(Gek::ProcessorType)
GEK_PLUGIN_END
