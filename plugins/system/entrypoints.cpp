#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\System\InputInterface.h"
#include "GEK\System\AudioInterface.h"
#include "GEK\System\VideoInterface.h"

HRESULT gekCheckResult(Gek::Context::Interface *context, LPCSTR file, UINT line, LPCSTR function, HRESULT resultValue)
{
    if (FAILED(resultValue))
    {
        context->logMessage(file, line, L"Call Failed (0x%08X): %S", resultValue, function);
    }

    return resultValue;
}

namespace Gek
{
    namespace Input
    {
        DECLARE_REGISTERED_CLASS(System);
    }; // namespace Input

    namespace Audio
    {
        DECLARE_REGISTERED_CLASS(System);
    }; // namespace Audio

    namespace Video
    {
        DECLARE_REGISTERED_CLASS(System);
    }; // namespace Video
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(System)
    ADD_CONTEXT_CLASS(Gek::Input::Class, Gek::Input::System)
    ADD_CONTEXT_CLASS(Gek::Audio::Class, Gek::Audio::System)
    ADD_CONTEXT_CLASS(Gek::Video::Class, Gek::Video::System)
END_CONTEXT_SOURCE