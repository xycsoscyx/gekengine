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
#include <concurrent_vector.h>
#include <memory>
#include <map>
#include <set>
#include <ppl.h>

#undef min

namespace Gek
{
    namespace Model
    {
        static const UINT32 MaxInstanceCount = 500;

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

            struct Data
            {
                bool loaded;
                bool ready;
                CStringW fileName;
                Shape::AlignedBox alignedBox;
                Handle positionHandle;
                Handle texCoordHandle;
                Handle normalHandle;
                Handle indexHandle;
                std::unordered_map<Handle, Material> materialList;

                Data(void)
                    : loaded(false)
                    , ready(false)
                {
                }
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
            Video3D::Interface *video;
            Engine::Render::Interface *render;
            Engine::Population::Interface *population;

            Handle pluginHandle;
            Handle instanceHandle;

            Handle nextModelHandle;
            concurrency::concurrent_unordered_map<Handle, Data> dataList;
            concurrency::concurrent_unordered_map<CStringW, Handle> dataNameList;
            concurrency::concurrent_unordered_map<Handle, Handle> dataEntityList;
            concurrency::concurrent_unordered_map<Handle, std::vector<Instance>> visibleList;

        public:
            System(void)
                : render(nullptr)
                , video(nullptr)
                , population(nullptr)
                , pluginHandle(InvalidHandle)
                , instanceHandle(InvalidHandle)
                , nextModelHandle(InvalidHandle)
            {
            }

            ~System(void)
            {
                BaseObservable::removeObserver(render, getClass<Engine::Render::Observer>());
                BaseObservable::removeObserver(population, getClass<Engine::Population::Observer>());
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(ObservableInterface)
                INTERFACE_LIST_ENTRY_COM(Engine::Population::Observer)
                INTERFACE_LIST_ENTRY_COM(Engine::Render::Observer)
                INTERFACE_LIST_ENTRY_COM(Engine::System::Interface)
            END_INTERFACE_LIST_USER

            HRESULT preLoadData(LPCWSTR fileName, Data &data)
            {
                static const UINT32 nPreReadSize = (sizeof(UINT32) + sizeof(UINT16) + sizeof(UINT16) + sizeof(Shape::AlignedBox));

                std::vector<UINT8> fileData;
                data.fileName.Format(L"%%root%%\\data\\models\\%s.gek", fileName);
                HRESULT resultValue = Gek::FileSystem::load(data.fileName, fileData, nPreReadSize);
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
                        data.fileName = fileName;
                        data.alignedBox = *(Gek::Shape::AlignedBox *)rawFileData;
                        resultValue = S_OK;
                    }
                }

                return resultValue;
            }

            HRESULT loadData(Data &data)
            {
                if (data.loaded)
                {
                    return S_OK;
                }

                data.loaded = true;
                std::vector<UINT8> fileData;
                HRESULT resultValue = Gek::FileSystem::load(data.fileName, fileData);
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
                        data.alignedBox = *(Gek::Shape::AlignedBox *)rawFileData;
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

                if (SUCCEEDED(resultValue))
                {
                    data.ready = true;
                }

                return resultValue;
            }

            Handle getModelHandle(LPCWSTR fileName)
            {
                Handle modelHandle = InvalidHandle;
                auto dataNameIterator = dataNameList.find(fileName);
                if (dataNameIterator != dataNameList.end())
                {
                    modelHandle = (*dataNameIterator).second;
                }
                else
                {
                    modelHandle = InterlockedIncrement(&nextModelHandle);
                    dataNameList[fileName] = modelHandle;
                    preLoadData(fileName, dataList[modelHandle]);
                }

                return modelHandle;
            }

            // System::Interface
            STDMETHODIMP initialize(IUnknown *initializerContext)
            {
                REQUIRE_RETURN(initializerContext, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComQIPtr<Video3D::Interface> video(initializerContext);
                CComQIPtr<Engine::Render::Interface> render(initializerContext);
                CComQIPtr<Engine::Population::Interface> population(initializerContext);
                if (render && video && population)
                {
                    this->video = video;
                    this->render = render;
                    this->population = population;
                    resultValue = BaseObservable::addObserver(population, getClass<Engine::Population::Observer>());
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = BaseObservable::addObserver(render, getClass<Engine::Render::Observer>());
                }

                if (SUCCEEDED(resultValue))
                {
                    pluginHandle = render->loadPlugin(L"model");
                }

                return resultValue;
            };

            // Population::Observer
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
                REQUIRE_VOID_RETURN(video);

                for (auto &data : dataList)
                {
                    video->freeResource(data.second.positionHandle);
                    video->freeResource(data.second.texCoordHandle);
                    video->freeResource(data.second.normalHandle);
                    video->freeResource(data.second.indexHandle);
                }

                dataList.clear();
                dataNameList.clear();
                dataEntityList.clear();
            }

            STDMETHODIMP_(void) onEntityCreated(Handle entityHandle)
            {
                REQUIRE_VOID_RETURN(population);

                if (population->hasComponent(entityHandle, Model::identifier) &&
                    population->hasComponent(entityHandle, Engine::Components::Transform::identifier))
                {
                    auto &modelComponent = population->getComponent<Model::Data>(entityHandle, Model::identifier);
                    auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(entityHandle, Engine::Components::Transform::identifier);
                    dataEntityList[entityHandle] = getModelHandle(modelComponent);
                }
            }

            STDMETHODIMP_(void) onEntityDestroyed(Handle entityHandle)
            {
                auto dataEntityIterator = dataEntityList.find(entityHandle);
                if (dataEntityIterator != dataEntityList.end())
                {
                    dataEntityList.unsafe_erase(dataEntityIterator);
                }
            }

            // Render::Observer
            STDMETHODIMP_(void) onRenderBegin(Handle cameraHandle)
            {
            }

            STDMETHODIMP_(void) onCullScene(Handle cameraHandle, const Gek::Shape::Frustum &viewFrustum)
            {
                REQUIRE_VOID_RETURN(population);

                visibleList.clear();
                for (auto dataEntity : dataEntityList)
                {
                    auto dataIterator = dataList.find(dataEntity.second);
                    if (dataIterator != dataList.end())
                    {
                        Gek::Math::Float3 size(1.0f, 1.0f, 1.0f);
                        if (population->hasComponent(dataEntity.first, Engine::Components::Size::identifier))
                        {
                            size = population->getComponent<Engine::Components::Size::Data>(dataEntity.first, Engine::Components::Size::identifier);
                        }

                        Gek::Shape::AlignedBox alignedBox((*dataIterator).second.alignedBox);
                        alignedBox.minimum *= size;
                        alignedBox.maximum *= size;

                        auto &transformComponent = population->getComponent<Engine::Components::Transform::Data>(dataEntity.first, Engine::Components::Transform::identifier);
                        Shape::OrientedBox orientedBox(alignedBox, transformComponent.rotation, transformComponent.position);
                        if (viewFrustum.isVisible(orientedBox))
                        {
                            Gek::Math::Float4 color(1.0f, 1.0f, 1.0f, 1.0f);
                            if (population->hasComponent(dataEntity.first, Engine::Components::Color::identifier))
                            {
                                color = population->getComponent<Engine::Components::Color::Data>(dataEntity.first, Engine::Components::Color::identifier);
                            }

                            visibleList[dataEntity.second].push_back(Instance(orientedBox.matrix, size, color, viewFrustum.getDistance(orientedBox.matrix.translation)));
                        }
                    }
                }

                for (auto instancePair : visibleList)
                {
                    Data &data = dataList[instancePair.first];
                    if (SUCCEEDED(loadData(data)) && data.ready)
                    {
                        auto &instanceList = instancePair.second;
                        concurrency::parallel_sort(instanceList.begin(), instanceList.end(), [&](const Instance &leftInstance, const Instance &rightInstance) -> bool
                        {
                            return (leftInstance.distance < rightInstance.distance);
                        });
                    }
                    else
                    {
                        instancePair.second.clear();
                    }
                }
            }

            STDMETHODIMP_(void) onDrawScene(Handle cameraHandle, Gek::Video3D::ContextInterface *videoContext, UINT32 vertexAttributes)
            {
                REQUIRE_VOID_RETURN(video);
                REQUIRE_VOID_RETURN(videoContext);

                if (!(vertexAttributes & Engine::Render::Attribute::Position) &&
                    !(vertexAttributes & Engine::Render::Attribute::TexCoord) &&
                    !(vertexAttributes & Engine::Render::Attribute::Normal))
                {
                    return;
                }

                render->enablePlugin(pluginHandle);
                videoContext->getVertexSystem()->setResource(instanceHandle, 0);
                videoContext->setPrimitiveType(Video3D::PrimitiveType::TRIANGLELIST);
                for (auto instancePair : visibleList)
                {
                    Data &data = dataList[instancePair.first];
                    auto &instanceList = instancePair.second;

                    if (vertexAttributes & Engine::Render::Attribute::Position)
                    {
                        videoContext->setVertexBuffer(data.positionHandle, 0, 0);
                    }

                    if (vertexAttributes & Engine::Render::Attribute::TexCoord)
                    {
                        videoContext->setVertexBuffer(data.texCoordHandle, 1, 0);
                    }

                    if (vertexAttributes & Engine::Render::Attribute::Normal)
                    {
                        videoContext->setVertexBuffer(data.normalHandle, 2, 0);
                    }

                    videoContext->setIndexBuffer(data.indexHandle, 0);
                    for (UINT32 firstInstance = 0; firstInstance < instanceList.size(); firstInstance += MaxInstanceCount)
                    {
                        Instance *instanceBufferList = nullptr;
                        if (SUCCEEDED(video->mapBuffer(instanceHandle, (LPVOID *)&instanceBufferList)))
                        {
                            UINT32 instanceCount = std::min(MaxInstanceCount, (instanceList.size() - firstInstance));
                            memcpy(instanceBufferList, instanceList.data(), (sizeof(Instance) * instanceCount));
                            video->unmapBuffer(instanceHandle);

                            for(auto &material : data.materialList)
                            {
                                render->enableMaterial(material.first);
                                videoContext->drawInstancedIndexedPrimitive(material.second.indexCount, instanceCount, material.second.firstIndex, material.second.firstVertex, 0);
                            }
                        }
                    }
                }
            }

            STDMETHODIMP_(void) onRenderEnd(Handle cameraHandle)
            {
            }

            STDMETHODIMP_(void) onRenderOverlay(void)
            {
            }
        };

        REGISTER_CLASS(System)
    }; // namespace Model
}; // namespace Gek
