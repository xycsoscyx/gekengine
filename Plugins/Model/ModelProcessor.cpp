#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Shapes/OrientedBox.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/Allocator.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Processor.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Color.hpp"
#include "GEK/Model/Base.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <xmmintrin.h>
#include <algorithm>
#include <memory>
#include <future>
#include <ppl.h>
#include <array>
#include <map>

namespace Gek
{
    GEK_CONTEXT_USER(Model, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Model, Edit::Component>
    {
    public:
        Model(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Model const * const data, JSON::Object &componentData) const
        {
            componentData.set(L"name", data->name);
        }

        void load(Components::Model * const data, const JSON::Object &componentData)
        {
            data->name = getValue(componentData, L"name", String());
        }

        // Edit::Component
        bool ui(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &modelComponent = *dynamic_cast<Components::Model *>(data);
            bool changed = ImGui::Gek::InputString("Model", modelComponent.name, flags);
            ImGui::SetCurrentContext(nullptr);
            return changed;
        }

        void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
        }

        bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            return ui(guiContext, entity, data, 0);
        }
    };

    GEK_CONTEXT_USER(ModelProcessor, Plugin::Core *)
        , public Plugin::ProcessorMixin<ModelProcessor, Components::Model, Components::Transform>
        , public Plugin::Processor
    {
        GEK_ADD_EXCEPTION(InvalidModelIdentifier);
        GEK_ADD_EXCEPTION(InvalidModelType);
        GEK_ADD_EXCEPTION(InvalidModelVersion);

    public:
        struct Header
        {
            struct Part
            {
                wchar_t name[64];
                uint32_t vertexCount = 0;
                uint32_t indexCount = 0;
            };

            uint32_t identifier = 0;
            uint16_t type = 0;
            uint16_t version = 0;

            Shapes::AlignedBox boundingBox;

            uint32_t partCount = 0;
            Part partList[1];
        };

        struct Vertex
        {
            Math::Float3 position;
            Math::Float2 texCoord;
			Math::Float3 tangent;
			Math::Float3 biTangent;
			Math::Float3 normal;
        };

        struct Model
        {
            struct Part
            {
                MaterialHandle material;
                std::vector<ResourceHandle> vertexBufferList = std::vector<ResourceHandle>(5);
                ResourceHandle indexBuffer;
                uint32_t indexCount = 0;
            };

            Shapes::AlignedBox boundingBox;
            std::vector<Part> partList;
        };

        struct Data
        {
            Model *model = nullptr;
        };

        struct Instance
        {
            Math::Float4x4 transform;
        };

        struct DrawData
        {
            uint32_t instanceStart = 0;
            uint32_t instanceCount = 0;
            const Model::Part *part = nullptr;

            DrawData(uint32_t instanceStart = 0, uint32_t instanceCount = 0, const Model::Part *part = nullptr)
                : instanceStart(instanceStart)
                , instanceCount(instanceCount)
                , part(part)
            {
            }
        };

    private:
        Video::Device *videoDevice = nullptr;
        Plugin::Population *population = nullptr;
        Plugin::Resources *resources = nullptr;
        Plugin::Renderer *renderer = nullptr;

        VisualHandle visual;
        Video::BufferPtr instanceBuffer;
        ThreadPool loadPool;

        concurrency::concurrent_unordered_map<std::size_t, Model> modelMap;

        std::vector<float, AlignedAllocator<float, 16>> halfSizeXList;
        std::vector<float, AlignedAllocator<float, 16>> halfSizeYList;
        std::vector<float, AlignedAllocator<float, 16>> halfSizeZList;
        std::vector<float, AlignedAllocator<float, 16>> transformList[16];
        std::vector<bool> visibilityList;

    public:
        ModelProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , videoDevice(core->getVideoDevice())
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
            , loadPool(1)
        {
            GEK_REQUIRE(videoDevice);
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

            population->onEntityCreated.connect<ModelProcessor, &ModelProcessor::onEntityCreated>(this);
            population->onEntityDestroyed.connect<ModelProcessor, &ModelProcessor::onEntityDestroyed>(this);
            population->onComponentAdded.connect<ModelProcessor, &ModelProcessor::onComponentAdded>(this);
            population->onComponentRemoved.connect<ModelProcessor, &ModelProcessor::onComponentRemoved>(this);
            renderer->onQueueDrawCalls.connect<ModelProcessor, &ModelProcessor::onQueueDrawCalls>(this);

            visual = resources->loadVisual(L"model");

            Video::Buffer::Description instanceDescription;
            instanceDescription.stride = sizeof(Math::Float4x4);
            instanceDescription.count = 100;
            instanceDescription.type = Video::Buffer::Description::Type::Vertex;
            instanceDescription.flags = Video::Buffer::Description::Flags::Mappable;
            instanceBuffer = videoDevice->createBuffer(instanceDescription);
            instanceBuffer->setName(L"model:instances");
        }

        ~ModelProcessor(void)
        {
            renderer->onQueueDrawCalls.disconnect<ModelProcessor, &ModelProcessor::onQueueDrawCalls>(this);
            population->onComponentRemoved.disconnect<ModelProcessor, &ModelProcessor::onComponentRemoved>(this);
            population->onComponentAdded.disconnect<ModelProcessor, &ModelProcessor::onComponentAdded>(this);
            population->onEntityDestroyed.disconnect<ModelProcessor, &ModelProcessor::onEntityDestroyed>(this);
            population->onEntityCreated.disconnect<ModelProcessor, &ModelProcessor::onEntityCreated>(this);
        }

        void addEntity(Plugin::Entity * const entity)
        {
            try
            {
                ProcessorMixin::addEntity(entity, [&](auto &data, auto &modelComponent, auto &transformComponent) -> void
                {
                    String fileName(getContext()->getRootFileName(L"data", L"models", modelComponent.name).withExtension(L".gek"));
                    auto pair = modelMap.insert(std::make_pair(GetHash(modelComponent.name), Model()));
                    if (pair.second)
                    {
                        loadPool.enqueue([this, name = modelComponent.name, fileName, &model = pair.first->second](void) -> void
                        {
                            std::vector<uint8_t> buffer;
                            FileSystem::Load(fileName, buffer, sizeof(Header));

                            Header *header = (Header *)buffer.data();
                            if (header->identifier != *(uint32_t *)"GEKX")
                            {
                                throw InvalidModelIdentifier("Unknown model file identifier encountered");
                            }

                            if (header->type != 0)
                            {
                                throw InvalidModelType("Unsupported model type encountered");
                            }

                            if (header->version != 6)
                            {
                                throw InvalidModelVersion("Unsupported model version encountered");
                            }

                            model.boundingBox = header->boundingBox;
                            loadPool.enqueue([this, name = name, fileName, &model](void) -> void
                            {
                                std::vector<uint8_t> buffer;
                                FileSystem::Load(fileName, buffer);

                                Header *header = (Header *)buffer.data();
                                model.partList.resize(header->partCount);
                                uint8_t *bufferData = (uint8_t *)&header->partList[header->partCount];
                                for (uint32_t partIndex = 0; partIndex < header->partCount; ++partIndex)
                                {
                                    Header::Part &partHeader = header->partList[partIndex];
                                    Model::Part &part = model.partList[partIndex];
                                    if (wcslen(partHeader.name) > 0)
                                    {
                                        part.material = resources->loadMaterial(partHeader.name);
                                    }

                                    Video::Buffer::Description indexBufferDescription;
                                    indexBufferDescription.format = Video::Format::R16_UINT;
                                    indexBufferDescription.count = partHeader.indexCount;
                                    indexBufferDescription.type = Video::Buffer::Description::Type::Index;
                                    part.indexBuffer = resources->createBuffer(String::Format(L"model:indices:%v:%v", name, partIndex), indexBufferDescription, reinterpret_cast<uint16_t *>(bufferData));
                                    bufferData += (sizeof(uint16_t) * partHeader.indexCount);

                                    Video::Buffer::Description vertexBufferDescription;
                                    vertexBufferDescription.stride = sizeof(Math::Float3);
                                    vertexBufferDescription.count = partHeader.vertexCount;
                                    vertexBufferDescription.type = Video::Buffer::Description::Type::Vertex;
                                    part.vertexBufferList[0] = resources->createBuffer(String::Format(L"model:positions:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                    bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                    vertexBufferDescription.stride = sizeof(Math::Float2);
                                    part.vertexBufferList[1] = resources->createBuffer(String::Format(L"model:texcoords:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float2 *>(bufferData));
                                    bufferData += (sizeof(Math::Float2) * partHeader.vertexCount);

                                    vertexBufferDescription.stride = sizeof(Math::Float3);
                                    part.vertexBufferList[2] = resources->createBuffer(String::Format(L"model:tangents:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                    bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                    vertexBufferDescription.stride = sizeof(Math::Float3);
                                    part.vertexBufferList[3] = resources->createBuffer(String::Format(L"model:bitangents:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                    bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                    vertexBufferDescription.stride = sizeof(Math::Float3);
                                    part.vertexBufferList[4] = resources->createBuffer(String::Format(L"model:normals:%v:%v", name, partIndex), vertexBufferDescription, reinterpret_cast<Math::Float3 *>(bufferData));
                                    bufferData += (sizeof(Math::Float3) * partHeader.vertexCount);

                                    part.indexCount = partHeader.indexCount;
                                }
                            });
                        });
                    }

                    data.model = &pair.first->second;
                });
            }
            catch (...)
            {
            };
        }

        // Plugin::Population Slots
        void onEntityCreated(Plugin::Entity * const entity, wchar_t const * const entityName)
        {
            addEntity(entity);
        }

        void onEntityDestroyed(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }

        void onComponentAdded(Plugin::Entity * const entity, const std::type_index &type)
        {
            addEntity(entity);
        }

        void onComponentRemoved(Plugin::Entity * const entity, const std::type_index &type)
        {
            removeEntity(entity);
        }

        using InstanceList = concurrency::concurrent_vector<Math::Float4x4>;
        using PartMap = concurrency::concurrent_unordered_map<const Model::Part *, InstanceList>;
        using MaterialMap = concurrency::concurrent_unordered_map<MaterialHandle, PartMap>;

        struct SIMDVector
        {
            __m128 x, y, z, w;
        };

        struct SIMDMatrix
        {
            SIMDVector x, y, z, w;
        };

        SIMDMatrix simd_multiply(const SIMDMatrix &lhs, const SIMDMatrix &rhs)
        {
            SIMDVector x = simd_multiply(lhs.x, rhs);
            SIMDVector y = simd_multiply(lhs.y, rhs);
            SIMDVector z = simd_multiply(lhs.z, rhs);
            SIMDVector w = simd_multiply(lhs.w, rhs);
            SIMDMatrix res = { x, y, z, w };
            return res;
        }

        SIMDVector simd_multiply(const SIMDVector &v, const SIMDMatrix &m)
        {
            __m128 x = _mm_mul_ps(v.x, m.x.x);
            x = _mm_add_ps(_mm_mul_ps(v.y, m.y.x), x);
            x = _mm_add_ps(_mm_mul_ps(v.z, m.z.x), x);
            x = _mm_add_ps(_mm_mul_ps(v.w, m.w.x), x);

            __m128 y = _mm_mul_ps(v.x, m.x.y);
            y = x = _mm_add_ps(_mm_mul_ps(v.y, m.y.y), y);
            y = x = _mm_add_ps(_mm_mul_ps(v.z, m.z.y), y);
            y = x = _mm_add_ps(_mm_mul_ps(v.w, m.w.y), y);

            __m128 z = _mm_mul_ps(v.x, m.x.z);
            z = x = _mm_add_ps(_mm_mul_ps(v.y, m.y.z), z);
            z = x = _mm_add_ps(_mm_mul_ps(v.z, m.z.z), z);
            z = x = _mm_add_ps(_mm_mul_ps(v.w, m.w.z), z);

            __m128 w = _mm_mul_ps(v.x, m.x.w);
            w = x = _mm_add_ps(_mm_mul_ps(v.y, m.y.w), w);
            w = x = _mm_add_ps(_mm_mul_ps(v.z, m.z.w), w);
            w = x = _mm_add_ps(_mm_mul_ps(v.w, m.w.w), w);

            SIMDVector res =
            {
                x, y, z, w
            };

            return res;
        }

        void simd_min_max_transform(const SIMDMatrix &m, const SIMDVector &min, const SIMDVector &max, SIMDVector result[])
        {
            auto m_xx_x = _mm_mul_ps(m.x.x, min.x);    m_xx_x = _mm_add_ps(m_xx_x, m.w.x);
            auto m_xy_x = _mm_mul_ps(m.x.y, min.x);    m_xy_x = _mm_add_ps(m_xy_x, m.w.y);
            auto m_xz_x = _mm_mul_ps(m.x.z, min.x);    m_xz_x = _mm_add_ps(m_xz_x, m.w.z);
            auto m_xw_x = _mm_mul_ps(m.x.w, min.x);    m_xw_x = _mm_add_ps(m_xw_x, m.w.w);

            auto m_xx_X = _mm_mul_ps(m.x.x, max.x);    m_xx_X = _mm_add_ps(m_xx_X, m.w.x);
            auto m_xy_X = _mm_mul_ps(m.x.y, max.x);    m_xy_X = _mm_add_ps(m_xy_X, m.w.y);
            auto m_xz_X = _mm_mul_ps(m.x.z, max.x);    m_xz_X = _mm_add_ps(m_xz_X, m.w.z);
            auto m_xw_X = _mm_mul_ps(m.x.w, max.x);    m_xw_X = _mm_add_ps(m_xw_X, m.w.w);

            auto m_yx_y = _mm_mul_ps(m.y.x, min.y);
            auto m_yy_y = _mm_mul_ps(m.y.y, min.y);
            auto m_yz_y = _mm_mul_ps(m.y.z, min.y);
            auto m_yw_y = _mm_mul_ps(m.y.w, min.y);

            auto m_yx_Y = _mm_mul_ps(m.y.x, max.y);
            auto m_yy_Y = _mm_mul_ps(m.y.y, max.y);
            auto m_yz_Y = _mm_mul_ps(m.y.z, max.y);
            auto m_yw_Y = _mm_mul_ps(m.y.w, max.y);

            auto m_zx_z = _mm_mul_ps(m.z.x, min.z);
            auto m_zy_z = _mm_mul_ps(m.z.y, min.z);
            auto m_zz_z = _mm_mul_ps(m.z.z, min.z);
            auto m_zw_z = _mm_mul_ps(m.z.w, min.z);

            auto m_zx_Z = _mm_mul_ps(m.z.x, max.z);
            auto m_zy_Z = _mm_mul_ps(m.z.y, max.z);
            auto m_zz_Z = _mm_mul_ps(m.z.z, max.z);
            auto m_zw_Z = _mm_mul_ps(m.z.w, max.z);

            {
                auto xyz_x = _mm_add_ps(m_xx_x, m_yx_y);   xyz_x = _mm_add_ps(xyz_x, m_zx_z);
                auto xyz_y = _mm_add_ps(m_xy_x, m_yy_y);   xyz_y = _mm_add_ps(xyz_y, m_zy_z);
                auto xyz_z = _mm_add_ps(m_xz_x, m_yz_y);   xyz_z = _mm_add_ps(xyz_z, m_zz_z);
                auto xyz_w = _mm_add_ps(m_xw_x, m_yw_y);   xyz_w = _mm_add_ps(xyz_w, m_zw_z);
                result[0].x = xyz_x;
                result[0].y = xyz_y;
                result[0].z = xyz_z;
                result[0].w = xyz_w;
            }

            {
                auto Xyz_x = _mm_add_ps(m_xx_X, m_yx_y);   Xyz_x = _mm_add_ps(Xyz_x, m_zx_z);
                auto Xyz_y = _mm_add_ps(m_xy_X, m_yy_y);   Xyz_y = _mm_add_ps(Xyz_y, m_zy_z);
                auto Xyz_z = _mm_add_ps(m_xz_X, m_yz_y);   Xyz_z = _mm_add_ps(Xyz_z, m_zz_z);
                auto Xyz_w = _mm_add_ps(m_xw_X, m_yw_y);   Xyz_w = _mm_add_ps(Xyz_w, m_zw_z);
                result[1].x = Xyz_x;
                result[1].y = Xyz_y;
                result[1].z = Xyz_z;
                result[1].w = Xyz_w;
            }

            {
                auto xYz_x = _mm_add_ps(m_xx_x, m_yx_Y);   xYz_x = _mm_add_ps(xYz_x, m_zx_z);
                auto xYz_y = _mm_add_ps(m_xy_x, m_yy_Y);   xYz_y = _mm_add_ps(xYz_y, m_zy_z);
                auto xYz_z = _mm_add_ps(m_xz_x, m_yz_Y);   xYz_z = _mm_add_ps(xYz_z, m_zz_z);
                auto xYz_w = _mm_add_ps(m_xw_x, m_yw_Y);   xYz_w = _mm_add_ps(xYz_w, m_zw_z);
                result[2].x = xYz_x;
                result[2].y = xYz_y;
                result[2].z = xYz_z;
                result[2].w = xYz_w;
            }

            {
                auto XYz_x = _mm_add_ps(m_xx_X, m_yx_Y);   XYz_x = _mm_add_ps(XYz_x, m_zx_z);
                auto XYz_y = _mm_add_ps(m_xy_X, m_yy_Y);   XYz_y = _mm_add_ps(XYz_y, m_zy_z);
                auto XYz_z = _mm_add_ps(m_xz_X, m_yz_Y);   XYz_z = _mm_add_ps(XYz_z, m_zz_z);
                auto XYz_w = _mm_add_ps(m_xw_X, m_yw_Y);   XYz_w = _mm_add_ps(XYz_w, m_zw_z);
                result[3].x = XYz_x;
                result[3].y = XYz_y;
                result[3].z = XYz_z;
                result[3].w = XYz_w;
            }

            {
                auto xyZ_x = _mm_add_ps(m_xx_x, m_yx_y);   xyZ_x = _mm_add_ps(xyZ_x, m_zx_Z);
                auto xyZ_y = _mm_add_ps(m_xy_x, m_yy_y);   xyZ_y = _mm_add_ps(xyZ_y, m_zy_Z);
                auto xyZ_z = _mm_add_ps(m_xz_x, m_yz_y);   xyZ_z = _mm_add_ps(xyZ_z, m_zz_Z);
                auto xyZ_w = _mm_add_ps(m_xw_x, m_yw_y);   xyZ_w = _mm_add_ps(xyZ_w, m_zw_Z);
                result[4].x = xyZ_x;
                result[4].y = xyZ_y;
                result[4].z = xyZ_z;
                result[4].w = xyZ_w;
            }

            {
                auto XyZ_x = _mm_add_ps(m_xx_X, m_yx_y);   XyZ_x = _mm_add_ps(XyZ_x, m_zx_Z);
                auto XyZ_y = _mm_add_ps(m_xy_X, m_yy_y);   XyZ_y = _mm_add_ps(XyZ_y, m_zy_Z);
                auto XyZ_z = _mm_add_ps(m_xz_X, m_yz_y);   XyZ_z = _mm_add_ps(XyZ_z, m_zz_Z);
                auto XyZ_w = _mm_add_ps(m_xw_X, m_yw_y);   XyZ_w = _mm_add_ps(XyZ_w, m_zw_Z);
                result[5].x = XyZ_x;
                result[5].y = XyZ_y;
                result[5].z = XyZ_z;
                result[5].w = XyZ_w;
            }

            {
                auto xYZ_x = _mm_add_ps(m_xx_x, m_yx_Y);   xYZ_x = _mm_add_ps(xYZ_x, m_zx_Z);
                auto xYZ_y = _mm_add_ps(m_xy_x, m_yy_Y);   xYZ_y = _mm_add_ps(xYZ_y, m_zy_Z);
                auto xYZ_z = _mm_add_ps(m_xz_x, m_yz_Y);   xYZ_z = _mm_add_ps(xYZ_z, m_zz_Z);
                auto xYZ_w = _mm_add_ps(m_xw_x, m_yw_Y);   xYZ_w = _mm_add_ps(xYZ_w, m_zw_Z);
                result[6].x = xYZ_x;
                result[6].y = xYZ_y;
                result[6].z = xYZ_z;
                result[6].w = xYZ_w;
            }

            {
                auto XYZ_x = _mm_add_ps(m_xx_X, m_yx_Y);   XYZ_x = _mm_add_ps(XYZ_x, m_zx_Z);
                auto XYZ_y = _mm_add_ps(m_xy_X, m_yy_Y);   XYZ_y = _mm_add_ps(XYZ_y, m_zy_Z);
                auto XYZ_z = _mm_add_ps(m_xz_X, m_yz_Y);   XYZ_z = _mm_add_ps(XYZ_z, m_zz_Z);
                auto XYZ_w = _mm_add_ps(m_xw_X, m_yw_Y);   XYZ_w = _mm_add_ps(XYZ_w, m_zw_Z);
                result[7].x = XYZ_x;
                result[7].y = XYZ_y;
                result[7].z = XYZ_z;
                result[7].w = XYZ_w;
            }
        }

        void cull(Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix)
        {
            const auto viewProjectionMatrix(viewMatrix * projectionMatrix);
            const SIMDMatrix simd_view_proj =
            {
                _mm_set_ps1(viewProjectionMatrix.rx.x),
                _mm_set_ps1(viewProjectionMatrix.rx.y),
                _mm_set_ps1(viewProjectionMatrix.rx.z),
                _mm_set_ps1(viewProjectionMatrix.rx.w),

                _mm_set_ps1(viewProjectionMatrix.ry.x),
                _mm_set_ps1(viewProjectionMatrix.ry.y),
                _mm_set_ps1(viewProjectionMatrix.ry.z),
                _mm_set_ps1(viewProjectionMatrix.ry.w),

                _mm_set_ps1(viewProjectionMatrix.rz.x),
                _mm_set_ps1(viewProjectionMatrix.rz.y),
                _mm_set_ps1(viewProjectionMatrix.rz.z),
                _mm_set_ps1(viewProjectionMatrix.rz.w),

                _mm_set_ps1(viewProjectionMatrix.rw.x),
                _mm_set_ps1(viewProjectionMatrix.rw.y),
                _mm_set_ps1(viewProjectionMatrix.rw.z),
                _mm_set_ps1(viewProjectionMatrix.rw.w),
            };

            auto objectCount = getEntityCount();
            for (uint32_t entityIndex = 0; entityIndex < objectCount; entityIndex += 4)
            {
                // Load the world transform matrix for four objects via the indirection table.
                SIMDMatrix world;
                world.x.x = _mm_load_ps(&transformList[0][entityIndex]);
                world.x.y = _mm_load_ps(&transformList[1][entityIndex]);
                world.x.z = _mm_load_ps(&transformList[2][entityIndex]);
                world.x.w = _mm_load_ps(&transformList[3][entityIndex]);

                world.y.x = _mm_load_ps(&transformList[4][entityIndex]);
                world.y.y = _mm_load_ps(&transformList[5][entityIndex]);
                world.y.z = _mm_load_ps(&transformList[6][entityIndex]);
                world.y.w = _mm_load_ps(&transformList[7][entityIndex]);

                world.z.x = _mm_load_ps(&transformList[8][entityIndex]);
                world.z.y = _mm_load_ps(&transformList[9][entityIndex]);
                world.z.z = _mm_load_ps(&transformList[10][entityIndex]);
                world.z.w = _mm_load_ps(&transformList[11][entityIndex]);

                world.w.x = _mm_load_ps(&transformList[12][entityIndex]);
                world.w.y = _mm_load_ps(&transformList[13][entityIndex]);
                world.w.z = _mm_load_ps(&transformList[14][entityIndex]);
                world.w.w = _mm_load_ps(&transformList[15][entityIndex]);

                // Create the matrix to go from object->world->view->clip space.
                const auto clip = simd_multiply(world, simd_view_proj);

                const auto zero = _mm_setzero_ps();

                // Load the mininum and maximum corner positions of the bounding box in object space.
                SIMDVector min_pos;
                min_pos.x = _mm_sub_ps(zero, _mm_load_ps(&halfSizeXList[entityIndex]));
                min_pos.y = _mm_sub_ps(zero, _mm_load_ps(&halfSizeYList[entityIndex]));
                min_pos.z = _mm_sub_ps(zero, _mm_load_ps(&halfSizeZList[entityIndex]));
                min_pos.w = _mm_set_ps1(1.0f);

                SIMDVector max_pos;
                max_pos.x = _mm_load_ps(&halfSizeXList[entityIndex]);
                max_pos.y = _mm_load_ps(&halfSizeYList[entityIndex]);
                max_pos.z = _mm_load_ps(&halfSizeZList[entityIndex]);
                max_pos.w = _mm_set_ps1(1.0f);

                SIMDVector clip_pos[8];
                // Transform each bounding box corner from object to clip space by sharing calculations.
                simd_min_max_transform(clip, min_pos, max_pos, clip_pos);

                const auto all_true = _mm_cmpeq_ps(zero, zero);

                // Initialize test conditions.
                auto all_x_less = all_true;
                auto all_x_greater = all_true;
                auto all_y_less = all_true;
                auto all_y_greater = all_true;
                auto all_z_less = all_true;
                auto any_z_less = _mm_cmpgt_ps(zero, zero);
                auto all_z_greater = all_true;

                // Test each corner of the oobb and if any corner intersects the frustum that object
                // is visible.
                for (unsigned cs = 0; cs < 8; ++cs)
                {
                    const auto neg_cs_w = _mm_sub_ps(zero, clip_pos[cs].w);

                    auto x_le = _mm_cmple_ps(clip_pos[cs].x, neg_cs_w);
                    auto x_ge = _mm_cmpge_ps(clip_pos[cs].x, clip_pos[cs].w);
                    all_x_less = _mm_and_ps(x_le, all_x_less);
                    all_x_greater = _mm_and_ps(x_ge, all_x_greater);

                    auto y_le = _mm_cmple_ps(clip_pos[cs].y, neg_cs_w);
                    auto y_ge = _mm_cmpge_ps(clip_pos[cs].y, clip_pos[cs].w);
                    all_y_less = _mm_and_ps(y_le, all_y_less);
                    all_y_greater = _mm_and_ps(y_ge, all_y_greater);

                    auto z_le = _mm_cmple_ps(clip_pos[cs].z, zero);
                    auto z_ge = _mm_cmpge_ps(clip_pos[cs].z, clip_pos[cs].w);
                    all_z_less = _mm_and_ps(z_le, all_z_less);
                    all_z_greater = _mm_and_ps(z_ge, all_z_greater);
                    any_z_less = _mm_or_ps(z_le, any_z_less);
                }

                const auto any_x_outside = _mm_or_ps(all_x_less, all_x_greater);
                const auto any_y_outside = _mm_or_ps(all_y_less, all_y_greater);
                const auto any_z_outside = _mm_or_ps(all_z_less, all_z_greater);
                auto outside = _mm_or_ps(any_x_outside, any_y_outside);
                outside = _mm_or_ps(outside, any_z_outside);

                const auto inside = _mm_xor_ps(outside, all_true);

                __declspec(align(16)) uint32_t insideValues[4];
                _mm_store_ps((float *)insideValues, inside);
                for (uint32_t subIndex = 0; subIndex < 4; subIndex++)
                {
                    visibilityList[entityIndex + subIndex] = !insideValues[subIndex];
                }
            }
        }

        // Plugin::Renderer Slots
        void onQueueDrawCalls(const Shapes::Frustum &viewFrustum, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix)
        {
            GEK_REQUIRE(renderer);

            const auto entityCount = getEntityCount();
            auto buffer = (entityCount % 4);
            buffer = (buffer ? (4 - buffer) : buffer);
            halfSizeXList.reserve(entityCount + buffer);
            halfSizeYList.reserve(entityCount + buffer);
            halfSizeZList.reserve(entityCount + buffer);
            halfSizeXList.clear();
            halfSizeYList.clear();
            halfSizeZList.clear();

            for (size_t element = 0; element < 16; element++)
            {
                transformList[element].reserve(entityCount + buffer);
                transformList[element].clear();
            }

            listEntities([&](Plugin::Entity * const entity, auto &data, auto &modelComponent, auto &transformComponent) -> void
            {
                Model &model = *data.model;
                auto modelSize(model.boundingBox.maximum - model.boundingBox.minimum);
                auto modelCenter(model.boundingBox.minimum + (modelSize * 0.5f));

                Math::Float4x4 matrix(transformComponent.getMatrix());
                matrix.translation.xyz += modelCenter;

                halfSizeXList.push_back(modelSize.x);
                halfSizeYList.push_back(modelSize.y);
                halfSizeZList.push_back(modelSize.z);

                for (size_t element = 0; element < 16; element++)
                {
                    transformList[element].push_back(matrix.data[element]);
                }
            });

            visibilityList.resize(entityCount + buffer);
            cull(viewMatrix, projectionMatrix);

            MaterialMap materialMap;
            auto entitySearch = std::begin(entityDataMap);
            for (size_t entityIndex = 0; entityIndex < entityCount; entityIndex++)
            {
                if (visibilityList[entityIndex])
                {
                    auto entity = entitySearch->first;
                    auto &transformComponent = entity->getComponent<Components::Transform>();
                    auto &model = *entitySearch->second.model;
                    ++entitySearch;

                    auto modelViewMatrix(transformComponent.getMatrix() * viewMatrix);
                    concurrency::parallel_for_each(std::begin(model.partList), std::end(model.partList), [&](const Model::Part &part) -> void
                    {
                        auto &partMap = materialMap[part.material];
                        auto &instanceList = partMap[&part];
                        instanceList.push_back(modelViewMatrix);
                    });
                }
            }

            size_t maximumInstanceCount = 0;
            for (auto &materialPair : materialMap)
            {
                auto material = materialPair.first;
                auto &partMap = materialPair.second;

                size_t partInstanceCount = 0;
                for (auto &partPair : partMap)
                {
                    auto part = partPair.first;
                    auto &partInstanceList = partPair.second;
                    partInstanceCount += partInstanceList.size();
                }

                std::vector<DrawData> drawDataList;
                drawDataList.reserve(partMap.size());

                std::vector<Math::Float4x4> instanceList;
                instanceList.reserve(partInstanceCount);

                for (auto &partPair : partMap)
                {
                    auto part = partPair.first;
                    auto &partInstanceList = partPair.second;
                    drawDataList.push_back(DrawData(instanceList.size(), partInstanceList.size(), part));
                    instanceList.insert(std::end(instanceList), std::begin(partInstanceList), std::end(partInstanceList));
                }

                maximumInstanceCount = std::max(maximumInstanceCount, instanceList.size());
                renderer->queueDrawCall(visual, material, std::move([this, drawDataList = move(drawDataList), instanceList = move(instanceList)](Video::Device::Context *videoContext) -> void
                {
                    Math::Float4x4 *instanceData = nullptr;
                    if (videoDevice->mapBuffer(instanceBuffer.get(), instanceData))
                    {
                        std::copy(std::begin(instanceList), std::end(instanceList), instanceData);
                        videoDevice->unmapBuffer(instanceBuffer.get());

                        videoContext->setVertexBufferList({ instanceBuffer.get() }, 5);

                        for (auto &drawData : drawDataList)
                        {
                            resources->setVertexBufferList(videoContext, drawData.part->vertexBufferList, 0);
                            resources->setIndexBuffer(videoContext, drawData.part->indexBuffer, 0);
                            resources->drawInstancedIndexedPrimitive(videoContext, drawData.instanceCount, drawData.instanceStart, drawData.part->indexCount, 0, 0);
                        }
                    }
                }));
            }

            if (instanceBuffer->getDescription().count < maximumInstanceCount)
            {
                instanceBuffer = nullptr;
                Video::Buffer::Description instanceDescription;
                instanceDescription.stride = sizeof(Math::Float4x4);
                instanceDescription.count = maximumInstanceCount;
                instanceDescription.type = Video::Buffer::Description::Type::Vertex;
                instanceDescription.flags = Video::Buffer::Description::Flags::Mappable;
                instanceBuffer = videoDevice->createBuffer(instanceDescription);
                instanceBuffer->setName(L"model:instances");
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Model)
    GEK_REGISTER_CONTEXT_USER(ModelProcessor)
}; // namespace Gek
