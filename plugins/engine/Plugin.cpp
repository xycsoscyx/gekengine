#include "GEK\Engine\PluginInterface.h"
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
            static Video3D::ElementType getElementType(LPCWSTR elementClassString)
            {
                if (_wcsicmp(elementClassString, L"instance") == 0) return Video3D::ElementType::INSTANCE;
                /*else if (_wcsicmp(elementClassString, L"vertex") == 0) */ return Video3D::ElementType::VERTEX;
            }

            namespace Plugin
            {
                class System : public Context::BaseUser
                    , public Interface
                {
                private:
                    Video3D::Interface *video;
                    Handle geometryProgramHandle;
                    Handle vertexProgramHandle;

                public:
                    System(void)
                        : video(nullptr)
                        , geometryProgramHandle(InvalidHandle)
                        , vertexProgramHandle(InvalidHandle)
                    {
                    }

                    ~System(void)
                    {
                    }

                    BEGIN_INTERFACE_LIST(System)
                        INTERFACE_LIST_ENTRY_COM(Interface)
                    END_INTERFACE_LIST_USER

                    // Plugin::Interface
                    STDMETHODIMP initialize(IUnknown *initializerContext, LPCWSTR fileName)
                    {
                        REQUIRE_RETURN(initializerContext, E_INVALIDARG);
                        REQUIRE_RETURN(fileName, E_INVALIDARG);

                        HRESULT resultValue = E_FAIL;
                        CComQIPtr<Video3D::Interface> video(initializerContext);
                        if (video)
                        {
                            this->video = video;
                            resultValue = S_OK;
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            Gek::Xml::Document xmlDocument;
                            resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\plugins\\%s.xml", fileName));
                            if (SUCCEEDED(resultValue))
                            {
                                resultValue = E_INVALIDARG;
                                Gek::Xml::Node xmlPluginNode = xmlDocument.getRoot();
                                if (xmlPluginNode && xmlPluginNode.getType().CompareNoCase(L"plugin") == 0)
                                {
                                    Gek::Xml::Node xmlLayoutNode = xmlPluginNode.firstChildElement(L"layout");
                                    if (xmlLayoutNode)
                                    {
                                        resultValue = S_OK;
                                        Gek::Xml::Node xmlGeometryNode = xmlPluginNode.firstChildElement(L"geometry");
                                        if (xmlGeometryNode)
                                        {
                                            Gek::Xml::Node xmlProgramNode = xmlGeometryNode.firstChildElement(L"program");
                                            if (xmlProgramNode && xmlProgramNode.hasAttribute(L"source") && xmlProgramNode.hasAttribute(L"entry"))
                                            {
                                                CStringW programFileName = xmlProgramNode.getAttribute(L"source");
                                                CW2A programEntryPoint(xmlProgramNode.getAttribute(L"entry"));
                                                geometryProgramHandle = video->loadGeometryProgram(L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint);
                                                resultValue = (geometryProgramHandle == InvalidHandle ? E_FAIL : S_OK);
                                            }
                                            else
                                            {
                                                resultValue = E_FAIL;
                                            }
                                        }

                                        if (SUCCEEDED(resultValue))
                                        {
                                            std::vector<CStringA> elementNameList;
                                            std::vector<Video3D::InputElement> elementList;
                                            Gek::Xml::Node xmlElementNode = xmlLayoutNode.firstChildElement(L"element");
                                            while (xmlElementNode)
                                            {
                                                if (xmlElementNode.hasAttribute(L"format") &&
                                                    xmlElementNode.hasAttribute(L"name") &&
                                                    xmlElementNode.hasAttribute(L"index"))
                                                {
                                                    Video3D::InputElement element;
                                                    element.format = Video3D::getFormat(xmlElementNode.getAttribute(L"format"));
                                                    elementNameList.push_back((LPCSTR)CW2A(xmlElementNode.getAttribute(L"name")));
                                                    element.semanticName = elementNameList.back().GetString();
                                                    element.semanticIndex = Gek::String::getINT32(xmlElementNode.getAttribute(L"index"));
                                                    if (xmlElementNode.hasAttribute(L"slotclass") &&
                                                        xmlElementNode.hasAttribute(L"slotindex"))
                                                    {
                                                        element.slotClass = getElementType(xmlElementNode.getAttribute(L"slotclass"));
                                                        element.slotIndex = Gek::String::getINT32(xmlElementNode.getAttribute(L"slotindex"));
                                                    }

                                                    elementList.push_back(element);
                                                }
                                                else
                                                {
                                                    break;
                                                }

                                                xmlElementNode = xmlElementNode.nextSiblingElement(L"element");
                                            };

                                            resultValue = E_INVALIDARG;
                                            Gek::Xml::Node xmlVertexNode = xmlPluginNode.firstChildElement(L"vertex");
                                            if (xmlVertexNode)
                                            {
                                                Gek::Xml::Node xmlProgramNode = xmlVertexNode.firstChildElement(L"program");
                                                if (xmlProgramNode && xmlProgramNode.hasAttribute(L"source") && xmlProgramNode.hasAttribute(L"entry"))
                                                {
                                                    CStringW programFileName = xmlProgramNode.getAttribute(L"source");
                                                    CW2A programEntryPoint(xmlProgramNode.getAttribute(L"entry"));
                                                    vertexProgramHandle = video->loadVertexProgram(L"%root%\\data\\programs\\" + programFileName + L".hlsl", programEntryPoint, elementList);
                                                    resultValue = S_OK;
                                                }
                                                else
                                                {
                                                    resultValue = E_FAIL;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        return resultValue;
                    }
                };

                REGISTER_CLASS(System)
            }; // namespace Plugin
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
