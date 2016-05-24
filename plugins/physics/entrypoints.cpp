#include <initguid.h>
#include <cguid.h>

#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

GEK_PLUGIN_BEGIN(Components)
    GEK_PLUGIN_CLASS(Gek::MassRegistration, Gek::MassImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::RigidBodyRegistration, Gek::RigidBodyImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::StaticBodyRegistration, Gek::StaticBodyImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::PlayerBodyRegistration, Gek::PlayerBodyImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::NewtonProcessorRegistration, Gek::NewtonProcessorImplementation)
        GEK_PLUGIN_TYPE(Gek::ProcessorType)
GEK_PLUGIN_END
