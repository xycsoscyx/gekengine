#include "GEK\Engine\Material.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoSystem.h"
#include <set>
#include <ppl.h>

namespace Gek
{
    class MaterialImplementation 
        : public ContextRegistration<MaterialImplementation, Resources *, const wchar_t *>
        , public Material
    {
    private:
        Resources *resources;
        std::list<ResourceHandle> resourceList;
        ShaderHandle shader;

    public:
        MaterialImplementation(Context *context, Resources *resources, const wchar_t *fileName)
            : ContextRegistration(context)
            , resources(resources)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(fileName));
            GEK_REQUIRE(resources);

            XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\materials\\%v.xml", fileName)));
            XmlNodePtr materialNode = document->getRoot(L"material");

            XmlNodePtr shaderNode = materialNode->firstChildElement(L"shader");
            String shaderFileName = shaderNode->getText();
            shader = resources->loadShader(shaderFileName);

            std::unordered_map<String, Shader::Resource> resourceMap;
            XmlNodePtr mapsNode = materialNode->firstChildElement(L"maps");
            XmlNodePtr mapNode = mapsNode->firstChildElement();
            while (mapNode->isValid())
            {
                auto &resource = resourceMap[mapNode->getType()];
                if (mapNode->hasAttribute(L"file"))
                {
                    resource.type = Shader::Resource::Type::File;
                    resource.fileName = mapNode->getAttribute(L"file");
                }
                else
                {
                    resource.type = Shader::Resource::Type::Data;
                    resource.fileName = mapNode->getAttribute(L"data");
                }

                mapNode = mapNode->nextSiblingElement();
            };

            resources->loadResourceList(shader, fileName, resourceMap, resourceList);
        }

        ~MaterialImplementation(void)
        {
        }

        // Material
        ShaderHandle getShader(void) const
        {
            return shader;
        }

        const std::list<ResourceHandle> &getResourceList(void) const
        {
            return resourceList;
        }
    };

    GEK_REGISTER_CONTEXT_USER(MaterialImplementation);
}; // namespace Gek
