#include "GEK\Engine\MaterialInterface.h"
#include "GEK\Engine\ShaderInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\System\VideoSystem.h"
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
                class System : public ContextUserMixin
                    , public Interface
                {
                private:
                    Video::Interface *video;
                    Render::Interface *render;
                    std::vector<CComPtr<Video::Texture::Interface>> mapList;
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
                        INTERFACE_LIST_ENTRY_COM(Engine::Render::Material::Interface)
                    END_INTERFACE_LIST_USER

                    // Interface
                    STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
                    {
                        gekLogScope(fileName);

                        REQUIRE_RETURN(initializerContext, E_INVALIDARG);
                        REQUIRE_RETURN(fileName, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        CComQIPtr<Video::Interface> video(initializerContext);
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

                                        CComPtr<IUnknown> shader;
                                        resultValue = render->loadShader(&shader, shaderFileName);
                                        if (shader)
                                        {
                                            resultValue = shader->QueryInterface(IID_PPV_ARGS(&this->shader));
                                            if (this->shader)
                                            {
                                                resultValue = this->shader->getMaterialValues(fileName, xmlMaterialNode, mapList);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        return resultValue;
                    }

                    STDMETHODIMP_(Shader::Interface *) getShader(void)
                    {
                        return shader;
                    }

                    STDMETHODIMP_(void) enable(Video::Context::Interface *context, LPCVOID passData)
                    {
                        REQUIRE_VOID_RETURN(shader);
                        shader->setMaterialValues(context, passData, mapList);
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Material
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
