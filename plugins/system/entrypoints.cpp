#include "GEK\Context\ContextUser.h"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(InputSystemImplementation);
    GEK_DECLARE_CONTEXT_USER(AudioSystemImplementation);
    GEK_DECLARE_CONTEXT_USER(VideoSystemImplementation);

    GEK_CONTEXT_BEGIN(System);
        GEK_CONTEXT_ADD_CLASS(L"InputSystem", InputSystemImplementation);
        GEK_CONTEXT_ADD_CLASS(L"AudioSystem", AudioSystemImplementation);
        GEK_CONTEXT_ADD_CLASS(L"VideoSystem", VideoSystemImplementation);
    GEK_CONTEXT_END();
}; // namespace Gek