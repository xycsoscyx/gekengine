#include "GEK\Context\BaseUser.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace Gek
{
    namespace Engine
    {
        namespace Render
        {
            class Program : public Context::BaseUser
            {
            public:

            private:
                Video3D::Interface *video;

            public:
                Program(void)
                    : video(nullptr)
                {
                }

                ~Program(void)
                {
                }

                BEGIN_INTERFACE_LIST(Program)
                END_INTERFACE_LIST_USER

                // Interface
                STDMETHODIMP initialize(IUnknown *initializerContext)
                {
                    REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                    HRESULT resultValue = E_FAIL;
                    CComQIPtr<Video3D::Interface> video(initializerContext);
                    if (video)
                    {
                        this->video = video;
                        resultValue = S_OK;
                    }

                    return resultValue;
                }
            };

            REGISTER_CLASS(Program)
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
