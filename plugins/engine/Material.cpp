#include "GEK\Engine\MaterialInterface.h"
#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Context\UserMixin.h"
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
                class System : public Context::UserMixin
                    , public Interface
                {
                private:
                    Video3D::Interface *video;
                    Render::Interface *render;
                    std::vector<CComPtr<Video3D::TextureInterface>> mapList;
                    std::vector<UINT32> propertyList;
                    CComPtr<Shader::Interface> shader;

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

                                        CComPtr<IUnknown> shader;
                                        resultValue = render->loadShader(&shader, shaderFileName);
                                        if (shader)
                                        {
                                            resultValue = shader->QueryInterface(IID_PPV_ARGS(&this->shader));
                                            if (this->shader)
                                            {
                                                resultValue = this->shader->getMaterialValues(fileName, xmlMaterialNode, mapList, propertyList);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        gekCheckResult(resultValue);
                        return resultValue;
                    }

                    STDMETHODIMP getShader(IUnknown **returnObject)
                    {
                        REQUIRE_RETURN(returnObject, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        if (shader)
                        {
                            resultValue = shader->QueryInterface(IID_PPV_ARGS(returnObject));
                        }

                        return resultValue;
                    }

                    STDMETHODIMP_(void) enable(Video3D::ContextInterface *context, LPCVOID passData)
                    {
                        REQUIRE_VOID_RETURN(shader);
                        shader->setMaterialValues(context, passData, mapList, propertyList);
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Material
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
