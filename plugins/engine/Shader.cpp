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
            class Shader : public Context::BaseUser
            {
            public:
                struct Map
                {
                    CStringW name;
                };

                enum class PropertyType : UINT8
                {
                    Float = 0,
                    Float2,
                    Float3,
                    Float4,
                };

                struct Property
                {
                    CStringW name;
                    PropertyType propertyType;
                };

            private:
                Video3D::Interface *video;
                std::vector<Map> mapList;
                std::vector<Property> propertyList;

            public:
                Shader(void)
                    : video(nullptr)
                {
                }

                ~Shader(void)
                {
                }

                BEGIN_INTERFACE_LIST(Shader)
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

            REGISTER_CLASS(Shader)
        }; // namespace Redner
    }; // namespace Engine
}; // namespace Gek
