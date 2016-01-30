#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(ShapeImplementation);
    DECLARE_INTERFACE_IID(ShapeRegistration, "46F85CD6-6879-4699-87E3-7C75559F66CC");

    DECLARE_REGISTERED_CLASS(ShapeProcessorImplementation);
    DECLARE_INTERFACE_IID(ShapeProcessorRegistration, "61173484-93D4-40AA-88BF-BE41154F1D10");
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::ShapeRegistration, Gek::ShapeImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::ShapeProcessorRegistration, Gek::ShapeProcessorImplementation)
        ADD_CLASS_TYPE(Gek::ProcessorType)
END_CONTEXT_SOURCE