#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\COM.h"
#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(MassImplementation);
    DECLARE_INTERFACE_IID(MassRegistration, "06FD2921-89A3-490C-BBF6-9C909D921E96");

    DECLARE_REGISTERED_CLASS(RigidBodyImplementation);
    DECLARE_INTERFACE_IID(RigidBodyRegistration, "5C52FBF3-261D-4016-9881-80E418C8E639");

    DECLARE_REGISTERED_CLASS(StaticBodyImplementation);
    DECLARE_INTERFACE_IID(StaticBodyRegistration, "D28C70AF-E77D-4D5D-BE58-FACDACE45DEA");

    DECLARE_REGISTERED_CLASS(PlayerBodyImplementation);
    DECLARE_INTERFACE_IID(PlayerBodyRegistration, "EF85AE66-9544-4B7C-A2CF-B2410BDA63BD");

    DECLARE_REGISTERED_CLASS(NewtonProcessorImplementation);
    DECLARE_INTERFACE_IID(NewtonProcessorRegistration, "BCF7CC29-F375-4F29-9DA7-921F6A78E4E3");
}; // namespace Gek

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
