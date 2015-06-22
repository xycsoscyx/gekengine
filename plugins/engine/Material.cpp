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
                    std::vector<CComPtr<Video3D::TextureInterface>> mapList;
                    std::vector<UINT32> propertyList;
                    CComPtr<IUnknown> shader;

                public:
                    System(void)
                        : video(nullptr)
                        , render(nullptr)
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
                        gekLogScope(__FUNCTION__);
                        gekLogParameter("%s", fileName);

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
                            gekCheckResult(resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\materials\\%s.xml", fileName)));
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
                                        resultValue = render->loadShader(&shader, shaderFileName);
                                        if (shader)
                                        {
                                            CComQIPtr<Shader::Interface> shader(this->shader);
                                            if (shader)
                                            {
                                                resultValue = shader->getMaterialValues(fileName, xmlMaterialNode, mapList, propertyList);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        gekCheckResult(resultValue);
                        return resultValue;
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Material
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
