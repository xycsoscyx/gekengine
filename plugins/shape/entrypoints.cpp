#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\COM.h"
#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(ShapeImplementation);
    DECLARE_INTERFACE_IID(ShapeRegistration, "46F85CD6-6879-4699-87E3-7C75559F66CC");

    DECLARE_REGISTERED_CLASS(ShapeProcessorImplementation);
    DECLARE_INTERFACE_IID(ShapeProcessorRegistration, "61173484-93D4-40AA-88BF-BE41154F1D10");
}; // namespace Gek

GEK_PLUGIN_BEGIN(Components)
    GEK_PLUGIN_CLASS(Gek::ShapeRegistration, Gek::ShapeImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::ShapeProcessorRegistration, Gek::ShapeProcessorImplementation)
        GEK_PLUGIN_TYPE(Gek::ProcessorType)
GEK_PLUGIN_END
