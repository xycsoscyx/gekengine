#include "GEK\Engine\MaterialInterface.h"
#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Engine\RenderInterface.h"
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
            namespace Material
            {
                class System : public Context::BaseUser
                    , public Interface
                {
                private:
                    Video3D::Interface *video;
                    Render::Interface *render;
                    std::vector<Handle> mapList;
                    std::vector<float> propertyList;
                    Handle shaderHandle;

                public:
                    System(void)
                        : video(nullptr)
                        , render(nullptr)
                        , shaderHandle(InvalidHandle)
                    {
                    }

                    ~System(void)
                    {
                    }

                    BEGIN_INTERFACE_LIST(System)
                        INTERFACE_LIST_ENTRY_COM(Interface)
                    END_INTERFACE_LIST_USER

                    // Interface
                    STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
                    {
                        REQUIRE_RETURN(initializerContext, E_INVALIDARG);
                        REQUIRE_RETURN(fileName, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        CComQIPtr<Video3D::Interface> video(initializerContext);
                        CComQIPtr<Render::Interface> render(initializerContext);
                        if (video && render)
                        {
                            this->video = video;
                            this->render = render;
                            resultValue = S_OK;
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            Gek::Xml::Document xmlDocument;
                            resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\materials\\%s.xml", fileName));
                            if (SUCCEEDED(resultValue))
                            {
                                resultValue = E_INVALIDARG;
                                Gek::Xml::Node xmlMaterialNode = xmlDocument.getRoot();
                                if (xmlMaterialNode && xmlMaterialNode.getType().CompareNoCase(L"material") == 0)
                                {
                                    Gek::Xml::Node xmlShaderNode = xmlMaterialNode.firstChildElement(L"shader");
                                    if (xmlShaderNode)
                                    {
                                        CStringW shaderFileName = xmlShaderNode.getText();
                                        if (render->loadShader(shaderFileName) != InvalidHandle)
                                        {
                                            shaderHandle = render->loadShader(shaderFileName);
                                            resultValue = S_OK;
                                        }
                                    }
                                }
                            }
                        }

                        return resultValue;
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Material
        }; // namespace Rdnder
    }; // namespace Engine
}; // namespace Gek
