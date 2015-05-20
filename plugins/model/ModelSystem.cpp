#include "GEK\Math\Matrix4x4.h"
#include "GEK\Shape\AlignedBox.h"
#include "GEK\Shape\OrientedBox.h"
#include "GEK\Utility\Common.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Engine\SystemInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Size.h"
#include "GEK\Components\Color.h"
#include "GEK\Engine\Model.h"
#include <concurrent_unordered_map.h>
#include <memory>
#include <map>
#include <set>

namespace Gek
{
    namespace Model
    {
        class System : public Context::BaseUser
            , public BaseObservable
            , public Engine::Population::Observer
            , public Engine::Render::Observer
            , public Engine::System::Interface
        {
        public:
            struct Material
            {
                UINT32 firstVertex;
                UINT32 firstIndex;
                UINT32 indexCount;
            };

            struct Data : public Shape::AlignedBox
            {
                Handle positionHandle;
                Handle texCoordHandle;
                Handle normalHandle;
                Handle indexHandle;
                std::unordered_map<Handle, Material> materialList;
            };

            struct Instance
            {
                Math::Float4x4 matrix;
                Math::Float3 size;
                Math::Float4 color;
                float distance;

                Instance(const Math::Float4x4 matrix, const Math::Float3 &size, const Math::Float4 &color, float distance)
                    : matrix(matrix)
                    , size(size)
                    , color(color)
                    , distance(distance)
                {
                }
            };

        private:
            Engine::Render::Interface *render;
            Video3D::Interface *video;
            Engine::Population::Interface *population;

            Handle programHandle;
            Handle instanceHandle;

            Handle nextModelHandle;
            concurrency::concurrent_unordered_map<Handle, Data> dataList;
            concurrency::concurrent_unordered_map<CStringW, Handle> dataNameList;
            concurrency::concurrent_unordered_map<Handle, Handle> entityList;
            concurrency::concurrent_unordered_map<Handle, Instance> visibleList;

        public:
            System(void)
                : render(nullptr)
                , video(nullptr)
                , population(nullptr)
                , programHandle(InvalidHandle)
                , instanceHandle(InvalidHandle)
                , nextModelHandle(InvalidHandle)
            {
            }

            ~System(void)
            {
                BaseObservable::removeObserver(population, getClass<Engine::Population::Observer>());
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(ObservableInterface)
                INTERFACE_LIST_ENTRY_COM(Engine::Population::Observer)
                INTERFACE_LIST_ENTRY_COM(Engine::Render::Observer)
            END_INTERFACE_LIST_USER

            HRESULT loadData(LPCWSTR fileName, Data &data)
            {
                std::vector<UINT8> fileData;
                HRESULT resultValue = Gek::FileSystem::load(Gek::String::format(L"%%root%%\\data\\models\\%s.gek", fileName), fileData);
                if (SUCCEEDED(resultValue))
                {
                    UINT8 *rawFileData = fileData.data();
                    UINT32 gekIdentifier = *((UINT32 *)rawFileData);
                    rawFileData += sizeof(UINT32);

                    UINT16 gekModelType = *((UINT16 *)rawFileData);
                    rawFileData += sizeof(UINT16);

                    UINT16 gekModelVersion = *((UINT16 *)rawFileData);
                    rawFileData += sizeof(UINT16);

                    resultValue = E_INVALIDARG;
                    if (gekIdentifier == *(UINT32 *)"GEKX" && gekModelType == 0 && gekModelVersion == 2)
                    {
                        Gek::Shape::AlignedBox alignedBox = *(Gek::Shape::AlignedBox *)rawFileData;
                        rawFileData += sizeof(Gek::Shape::AlignedBox);

                        UINT32 materialCount = *((UINT32 *)rawFileData);
                        rawFileData += sizeof(UINT32);

                        for (UINT32 materialIndex = 0; materialIndex < materialCount; ++materialIndex)
                        {
                            CStringA materialNameUtf8(rawFileData);
                            rawFileData += (materialNameUtf8.GetLength() + 1);
                            Handle materialHandle = render->loadMaterial(CA2W(materialNameUtf8, CP_UTF8));
                            if (materialHandle == InvalidHandle)
                            {
                                resultValue = E_FAIL;
                                break;
                            }

                            Material &material = data.materialList[materialHandle];
                            material.firstVertex = *((UINT32 *)rawFileData);
                            rawFileData += sizeof(UINT32);

                            material.firstIndex = *((UINT32 *)rawFileData);
                            rawFileData += sizeof(UINT32);

                            material.indexCount = *((UINT32 *)rawFileData);
                            rawFileData += sizeof(UINT32);
                        }

                        if (SUCCEEDED(resultValue))
                        {
                            UINT32 vertexCount = *((UINT32 *)rawFileData);
                            rawFileData += sizeof(UINT32);

                            if (SUCCEEDED(resultValue))
                            {
                                data.positionHandle = video->createBuffer(sizeof(Math::Float3), vertexCount, Video3D::BufferFlags::VERTEX_BUFFER | Video3D::BufferFlags::STATIC, rawFileData);
                                rawFileData += (sizeof(Math::Float3) * vertexCount);
                            }

                            if (SUCCEEDED(resultValue))
                            {
                                data.texCoordHandle = video->createBuffer(sizeof(Math::Float2), vertexCount, Video3D::BufferFlags::VERTEX_BUFFER | Video3D::BufferFlags::STATIC, rawFileData);
                                rawFileData += (sizeof(Math::Float2) * vertexCount);
                            }

                            if (SUCCEEDED(resultValue))
                            {
                                data.normalHandle = video->createBuffer(sizeof(Math::Float3), vertexCount, Video3D::BufferFlags::VERTEX_BUFFER | Video3D::BufferFlags::STATIC, rawFileData);
                                rawFileData += (sizeof(Math::Float3) * vertexCount);
                            }

                            if (SUCCEEDED(resultValue))
                            {
                                UINT32 indexCount = *((UINT32 *)rawFileData);
                                rawFileData += sizeof(UINT32);

                                data.indexHandle = video->createBuffer(sizeof(UINT16), indexCount, Video3D::BufferFlags::INDEX_BUFFER | Video3D::BufferFlags::STATIC, rawFileData);
                                rawFileData += (sizeof(UINT16) * indexCount);
                            }
                        }
                    }
                }

                return resultValue;
            }

            Handle getModelHandle(LPCWSTR fileName)
            {
                Handle modelHandle = InvalidHandle;
                auto modelNameIterator = dataNameList.find(fileName);
                if (modelNameIterator != dataNameList.end())
                {
                    modelHandle = (*modelNameIterator).second;
                }
                else
                {
                    modelHandle = InterlockedIncrement(&nextModelHandle);
                    dataNameList[fileName] = modelHandle;
                }

                return modelHandle;
            }

            // System::Interface
            STDMETHODIMP initialize(IUnknown *initializerContext)
            {
                REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComQIPtr<Engine::Render::Interface> render(initializerContext);
                CComQIPtr<Video3D::Interface> video(render);
                CComQIPtr<Engine::Population::Interface> population(initializerContext);
                if (render && video && population)
                {
                    this->render = render;
                    this->video = video;
                    this->population = population;
                    resultValue = BaseObservable::addObserver(population, getClass<Engine::Population::Observer>());
                }

                if (SUCCEEDED(resultValue))
                {
                    programHandle = render->loadProgram(L"model");
                }

                return resultValue;
            };

            // Population::ObserverInterface
            STDMETHODIMP_(void) onLoadBegin(void)
            {
            }

            STDMETHODIMP_(void) onLoadEnd(HRESULT resultValue)
            {
                if (FAILED(resultValue))
                {
                    onFree();
                }
            }

            STDMETHODIMP_(void) onFree(void)
            {
                dataList.clear();
                dataNameList.clear();
                entityList.clear();
            }

            STDMETHODIMP_(void) onEntityCreated(Handle entityHandle)
            {
                if (population->hasComponent(entityHandle, Model::identifier) &&
                    population->hasComponent(entityHandle, Engine::Components::Transform::identifier))
                {
                    auto &modelComponent = population->getComponent<Model::Data>(entityHandle, Model::identifier);
                    auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(entityHandle, Engine::Components::Transform::identifier);
                    entityList[entityHandle] = getModelHandle(modelComponent);
                }
            }

            STDMETHODIMP_(void) onEntityDestroyed(Handle entityHandle)
            {
                auto entityModelIterator = entityList.find(entityHandle);
                if (entityModelIterator != entityList.end())
                {
                    entityList.unsafe_erase(entityModelIterator);
                }
            }

            STDMETHODIMP_(void) onUpdate(float frameTime)
            {
            }

            // Engine::Population::Observer
            STDMETHODIMP_(void) onRenderBegin(Handle viewerHandle)
            {
            }

            STDMETHODIMP_(void) onCullScene(Handle viewerHandle, const Gek::Shape::Frustum &viewFrustum)
            {
                visibleList.clear();
                for (auto entity : entityList)
                {
                    auto modelIterator = dataList.find(entity.second);
                    if (modelIterator != dataList.end())
                    {
                        Gek::Math::Float3 size(1.0f, 1.0f, 1.0f);
                        if (population->hasComponent(entity.first, Engine::Components::Size::identifier))
                        {
                            size = population->getComponent<Engine::Components::Size::Data>(entity.first, Engine::Components::Size::identifier);
                        }

                        Gek::Shape::AlignedBox alignedBox((*modelIterator).second);
                        alignedBox.minimum *= size;
                        alignedBox.maximum *= size;

                        auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(entity.first, Engine::Components::Transform::identifier);
                        Shape::OrientedBox orientedBox(alignedBox, transformComponent.rotation, transformComponent.position);
                        if (viewFrustum.isVisible(orientedBox))
                        {
                            Gek::Math::Float4 color(1.0f, 1.0f, 1.0f, 1.0f);
                            if (population->hasComponent(entity.first, Engine::Components::Color::identifier))
                            {
                                color = population->getComponent<Engine::Components::Color::Data>(entity.first, Engine::Components::Color::identifier);
                            }

                            visibleList.insert(std::make_pair(entity.first, Instance(orientedBox.matrix, size, color, viewFrustum.origin.getDistance(orientedBox.matrix.translation))));
                        }
                    }
                }
            }

            STDMETHODIMP_(void) onDrawScene(Handle viewerHandle, Gek::Video3D::ContextInterface *videoContext, UINT32 vertexAttributes)
            {
                for (auto instance : visibleList)
                {
                }
            }

            STDMETHODIMP_(void) onRenderEnd(Handle viewerHandle)
            {
            }

            STDMETHODIMP_(void) onRenderOverlay(void)
            {
            }
        };

        REGISTER_CLASS(System)
    }; // namespace Model
}; // namespace Gek
