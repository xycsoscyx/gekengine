#include "GEK\Engine\Material.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Render.h"
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
    class MaterialImplementation : public ContextUserMixin
        , public Material
    {
    private:
        VideoSystem *video;
        Render *render;
        std::vector<CComPtr<VideoTexture>> mapList;
        CComPtr<Shader> shader;

    public:
        MaterialImplementation(void)
            : video(nullptr)
            , render(nullptr)
        {
        }

        ~MaterialImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(MaterialImplementation)
            INTERFACE_LIST_ENTRY_COM(Material)
        END_INTERFACE_LIST_USER

        // Interface
        STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
        {
            gekLogScope(fileName);

            REQUIRE_RETURN(initializerContext, E_INVALIDARG);
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<VideoSystem> video(initializerContext);
            CComQIPtr<Render> render(initializerContext);
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

        STDMETHODIMP_(Shader *) getShader(void)
        {
            return shader;
        }

        STDMETHODIMP_(void) enable(VideoContext *context, LPCVOID passData)
        {
            REQUIRE_VOID_RETURN(shader);
            shader->setMaterialValues(context, passData, mapList);
        }
    };

    REGISTER_CLASS(MaterialImplementation)
}; // namespace Gek
