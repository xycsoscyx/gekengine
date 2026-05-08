#include "API/Engine/Core.hpp"
#include "API/Engine/Editor.hpp"
#include "API/Engine/Entity.hpp"
#include "API/Engine/Population.hpp"
#include "API/Engine/Processor.hpp"
#include "API/Engine/Visualizer.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Model/Base.hpp"
#include "GEK/Physics/Base.hpp"
#include "GEK/Physics/StaticBody.hpp"
#include "GEK/Shapes/AlignedBox.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include <dCollision/ndContactNotify.h>
#include <dCollision/ndShapeCompound.h>
#include <future>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

namespace Gek
{
    namespace Physics
    {
        extern BodyPtr createPlayerBody(Plugin::Core *core, Plugin::Population *population, World *World, Plugin::Entity *const entity);
        extern BodyPtr createRigidBody(World *world, Plugin::Entity *const entity);

        class BufferReader
        {
          private:
            uint8_t *buffer = nullptr;
            size_t size = 0;
            size_t index = 0;

          public:
            BufferReader(uint8_t *buffer, size_t size)
                : buffer(buffer), size(size)
            {
            }

            template <typename TYPE>
            bool canRead(uint32_t count = 1) const
            {
                size_t readSize = sizeof(TYPE) * static_cast<size_t>(count);
                return (index + readSize) <= size;
            }

            template <typename TYPE>
            TYPE *read(uint32_t count = 1)
            {
                if (!canRead<TYPE>(count))
                {
                    return nullptr;
                }

                TYPE *data = (TYPE *)(buffer + index);
                index += (sizeof(TYPE) * count);
                return data;
            }
        };

        GEK_CONTEXT_USER(Processor, Plugin::Core *)
        , public Plugin::Processor, public World
        {
          public:
            struct Header
            {
                uint32_t identifier = 0;
                uint16_t type = 0;
                uint16_t version = 0;
            };

            struct HullHeader : public Header
            {
                uint32_t pointCount;
            };

            struct TreeHeader : public Header
            {
                struct Material
                {
                    char name[64];
                };

                struct Mesh
                {
                    uint32_t materialIndex;
                    uint32_t faceCount;
                    uint32_t pointCount;
                };

                struct Face
                {
                    int32_t indices[3];
                };

                uint32_t materialCount;
                uint32_t meshCount;
            };

            struct Vertex
            {
                Math::Float3 position;
                Math::Float2 texCoord;
                Math::Float3 normal;
            };

            struct Material
            {
                uint32_t firstVertex;
                uint32_t firstIndex;
                uint32_t indexCount;
            };

            class ContactNotify : public ndContactNotify
            {
                Processor *processor;

              public:
                ContactNotify(ndScene *scene, Processor *processor)
                    : ndContactNotify(scene), processor(processor) {}
                void OnContactCallback(const ndContact *const contact, ndFloat32 timestep) const override
                {
                    const auto &points = contact->GetContactPoints();
                    using NodeType = ndList<ndContactMaterial, ndContainersFreeListAlloc<ndContactMaterial>>::ndNode;
                    for (NodeType *node = points.GetFirst(); node; node = node->GetNext())
                    {
                        const ndContactMaterial &cp = node->GetInfo();
                        Plugin::Entity *entity0 = nullptr;
                        Plugin::Entity *entity1 = nullptr;
                        ndBodyKinematic *body0 = (ndBodyKinematic *)contact->GetBody0();
                        ndBodyKinematic *body1 = (ndBodyKinematic *)contact->GetBody1();
                        for (auto &pair : processor->entityBodyMap)
                        {
                            if (pair.second && pair.second->getAsNewtonBody() == body0)
                                entity0 = pair.first;
                            if (pair.second && pair.second->getAsNewtonBody() == body1)
                                entity1 = pair.first;
                        }
                        if (!entity0 || !entity1)
                            continue;
                        Math::Float3 position(cp.m_point.m_x, cp.m_point.m_y, cp.m_point.m_z);
                        Math::Float3 normal(cp.m_normal.m_x, cp.m_normal.m_y, cp.m_normal.m_z);
                        processor->onCollision(entity0, position, normal, entity1);
                    }
                }
            };

            class NewtonWorld : public ndWorld
            {
              public:
                NewtonWorld(Processor *processor)
                {
                    SetContactNotify(new ContactNotify(this->GetScene(), processor));
                }
            };

          private:
            Plugin::Core *core = nullptr;
            Plugin::Population *population = nullptr;
            Plugin::Visualizer *renderer = nullptr;
            Edit::Events *events = nullptr;

            tbb::concurrent_vector<Surface> surfaceList;
            tbb::concurrent_unordered_map<std::size_t, uint32_t> surfaceIndexMap;
            NewtonWorld *newtonWorld = nullptr;
            ThreadPool loadPool;

            tbb::concurrent_unordered_map<Plugin::Entity *, Physics::Body *> entityBodyMap;
            tbb::concurrent_unordered_map<Hash, std::shared_future<ndShape *>> shapeFutureMap;

          public:
            Processor(Context * context, Plugin::Core * core)
                : ContextRegistration(context), core(core), population(core->getPopulation()), renderer(core->getVisualizer()), loadPool(5)
            {
                assert(core);
                assert(population);
                assert(renderer);

                core->onInitialized.connect(this, &Processor::onInitialized);
                core->onShutdown.connect(this, &Processor::onShutdown);
                population->onReset.connect(this, &Processor::onReset);
                population->onEntityCreated.connect(this, &Processor::onEntityCreated);
                population->onEntityDestroyed.connect(this, &Processor::onEntityDestroyed);
                population->onComponentAdded.connect(this, &Processor::onComponentAdded);
                population->onComponentRemoved.connect(this, &Processor::onComponentRemoved);
                population->onUpdate[50].connect(this, &Processor::onUpdate);
                renderer->onShowUserInterface.connect(this, &Processor::onShowUserInterface);

                onReset();
            }

            void clear(void)
            {
                if (newtonWorld)
                {
                    newtonWorld->Sync();

                    surfaceList.clear();
                    surfaceIndexMap.clear();

                    newtonWorld->CleanUp();

                    entityBodyMap.clear();
                    shapeFutureMap.clear();

                    delete newtonWorld;
                    newtonWorld = nullptr;
                }
            }

            Task scheduleLoadShape(std::shared_ptr<std::promise<ndShape *>> promise, Components::Model const &modelComponent)
            {
                co_await loadPool.schedule();

                ndShape *shape = nullptr;
                if (modelComponent.name == "#cube")
                {
                    getContext()->log(Context::Info, "Loading box shape for model: {}", modelComponent.name);
                    shape = new ndShapeBox(1.0f, 1.0f, 1.0f);
                }
                else if (modelComponent.name == "#sphere")
                {
                    getContext()->log(Context::Info, "Loading sphere shape for model: {}", modelComponent.name);
                    shape = new ndShapeSphere(1.0f);
                }
                else
                {
                    auto filePath = getContext()->findDataPath(FileSystem::CreatePath("physics", modelComponent.name).withExtension(".gek"));
                    getContext()->log(Context::Info, "Loading physics model from file: {}", filePath.getFileName());
                    std::vector<uint8_t> buffer(FileSystem::Load(filePath));
                    if (buffer.size() < sizeof(Header))
                    {
                        getContext()->log(Context::Error, "File too small to be physics model: {}", modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }

                    BufferReader reader(buffer.data(), buffer.size());
                    Header *header = reader.read<Header>(0);
                    if (!header)
                    {
                        getContext()->log(Context::Error, "Unable to read physics header: {}", modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }

                    if (header->identifier != *(uint32_t *)"GEKX")
                    {
                        getContext()->log(Context::Error, "Unknown model file identifier encountered: {}", modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }

                    if (header->version != 3)
                    {
                        getContext()->log(Context::Error, "Unsupported model version encountered (requires: 3, has: {}): {}", header->version, modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }

                    if (header->type == 1)
                    {
                        getContext()->log(Context::Info, "Loading convex hull for static scene: {}", modelComponent.name);
                        HullHeader *hullHeader = reader.read<HullHeader>();
                        if (!hullHeader)
                        {
                            getContext()->log(Context::Error, "Unable to read convex hull header: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        Math::Float3 *points = reader.read<Math::Float3>(hullHeader->pointCount);
                        if (!points)
                        {
                            getContext()->log(Context::Error, "Invalid convex hull point data in physics model: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        shape = new ndShapeConvexHull(hullHeader->pointCount, sizeof(Math::Float3), 0.0f, points->data);
                    }
                    else if (header->type == 2)
                    {
                        getContext()->log(Context::Info, "Loading tree mesh for static scene: {}", modelComponent.name);
                        TreeHeader *treeHeader = reader.read<TreeHeader>();
                        if (!treeHeader)
                        {
                            getContext()->log(Context::Error, "Unable to read tree mesh header: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        // Read materials
                        std::vector<std::string> materialNames;
                        for (uint32_t i = 0; i < treeHeader->materialCount; ++i)
                        {
                            TreeHeader::Material *mat = reader.read<TreeHeader::Material>();
                            if (!mat)
                            {
                                getContext()->log(Context::Error, "Invalid material data in tree physics model: {}", modelComponent.name);
                                promise->set_value(nullptr);
                                co_return;
                            }

                            materialNames.push_back(std::string(mat->name));
                        }

                        // Read meshes
                        std::vector<TreeHeader::Mesh> meshes;
                        for (uint32_t i = 0; i < treeHeader->meshCount; ++i)
                        {
                            TreeHeader::Mesh *mesh = reader.read<TreeHeader::Mesh>();
                            if (!mesh)
                            {
                                getContext()->log(Context::Error, "Invalid mesh header in tree physics model: {}", modelComponent.name);
                                promise->set_value(nullptr);
                                co_return;
                            }

                            meshes.push_back(*mesh);
                        }

                        // Collect all valid triangles into a flat list first, then split into
                        // chunks. Newton's CalculateAdjacent calls ForAllSectors with the full
                        // scene AABB, which uses an ndFixSizeArray<..., 512> traversal stack.
                        // Newton uses fixed-size traversal stacks of 512 entries in several
                        // paths, with push logic that can write one past end at exactly 512.
                        // Keep a generous margin below that limit.
                        static constexpr uint32_t MaxFacesPerChunk = 240;

                        struct Triangle { ndVector v[3]; };
                        std::vector<Triangle> allTriangles;
                        size_t invalidFaceCount = 0;

                        for (auto &mesh : meshes)
                        {
                            TreeHeader::Face *faces = reader.read<TreeHeader::Face>(mesh.faceCount);
                            Math::Float3 *meshPoints = reader.read<Math::Float3>(mesh.pointCount);
                            if (!faces || !meshPoints)
                            {
                                getContext()->log(Context::Error, "Invalid face/point data in tree physics model: {}", modelComponent.name);
                                promise->set_value(nullptr);
                                co_return;
                            }

                            for (uint32_t f = 0; f < mesh.faceCount; ++f)
                            {
                                int32_t i0 = faces[f].indices[0];
                                int32_t i1 = faces[f].indices[1];
                                int32_t i2 = faces[f].indices[2];

                                if (i0 < 0 || i1 < 0 || i2 < 0 ||
                                    static_cast<uint32_t>(i0) >= mesh.pointCount ||
                                    static_cast<uint32_t>(i1) >= mesh.pointCount ||
                                    static_cast<uint32_t>(i2) >= mesh.pointCount)
                                {
                                    ++invalidFaceCount;
                                    continue;
                                }

                                Triangle tri;
                                tri.v[0] = ndVector(meshPoints[i0].x, meshPoints[i0].y, meshPoints[i0].z, 0.0f);
                                tri.v[1] = ndVector(meshPoints[i1].x, meshPoints[i1].y, meshPoints[i1].z, 0.0f);
                                tri.v[2] = ndVector(meshPoints[i2].x, meshPoints[i2].y, meshPoints[i2].z, 0.0f);
                                allTriangles.push_back(tri);
                            }
                        }

                        if (invalidFaceCount > 0)
                        {
                            getContext()->log(Context::Warning, "Skipped {} invalid faces while loading tree physics model: {}", invalidFaceCount, modelComponent.name);
                        }

                        uint32_t totalFaces = static_cast<uint32_t>(allTriangles.size());
                        if (totalFaces == 0)
                        {
                            getContext()->log(Context::Error, "No valid faces in tree physics model: {}", modelComponent.name);
                            promise->set_value(nullptr);
                            co_return;
                        }

                        uint32_t chunkCount = (totalFaces + MaxFacesPerChunk - 1) / MaxFacesPerChunk;
                        getContext()->log(Context::Info, "Building BVH for {}: {} faces in {} chunk(s) of max {}", modelComponent.name, totalFaces, chunkCount, MaxFacesPerChunk);

                        auto* compound = new ndShapeCompound();
                        compound->BeginAddRemove();

                        for (uint32_t chunk = 0; chunk < chunkCount; ++chunk)
                        {
                            uint32_t start = chunk * MaxFacesPerChunk;
                            uint32_t end   = std::min(start + MaxFacesPerChunk, totalFaces);

                            ndPolygonSoupBuilder builder;
                            builder.Begin();
                            for (uint32_t f = start; f < end; ++f)
                            {
                                builder.AddFace(&allTriangles[f].v[0].m_x, sizeof(ndVector), 3, 0);
                            }
                            builder.End(false);

                            compound->AddCollision(new ndShapeInstance(new ndShapeStatic_bvh(builder)));
                        }

                        compound->EndAddRemove();
                        shape = compound;
                    }
                    else
                    {
                        getContext()->log(Context::Error, "Unsupported model type encountered: {}", modelComponent.name);
                        promise->set_value(nullptr);
                        co_return;
                    }
                }

                if (shape)
                {
                    getContext()->log(Context::Info, "Physics shape successfully loaded: {}", modelComponent.name);
                    // AddRef once here so the cache holds a permanent reference.
                    // ndShape starts with refCount=0; the ndShapeInstance copy constructor
                    // deep-copies ndShapeCompound and Releases the original, which would
                    // drop refCount back to 0 and delete the shape after the first use.
                    // Holding a permanent ref prevents that and keeps the cached pointer valid.
                    shape->AddRef();
                    promise->set_value(shape);
                }
                else
                {
                    getContext()->log(Context::Error, "Unable to create physics shape: {}", modelComponent.name);
                    promise->set_value(nullptr);
                }
            }

            ndShape *loadShape(Components::Model const &modelComponent)
            {
                auto hash = GetHash(modelComponent.name);
                auto promise = std::make_shared<std::promise<ndShape *>>();
                auto future = promise->get_future().share();
                auto shapeInsert = shapeFutureMap.insert(std::make_pair(hash, future));
                if (shapeInsert.second)
                {
                    fprintf(stderr, "[loadShape] scheduling NEW shape: %s\n", modelComponent.name.c_str()); fflush(stderr);
                    scheduleLoadShape(promise, modelComponent);
                }
                else
                {
                    fprintf(stderr, "[loadShape] reusing CACHED future: %s\n", modelComponent.name.c_str()); fflush(stderr);
                }

                auto shapeFuture = shapeInsert.first->second;
                fprintf(stderr, "[loadShape] calling .get() for: %s\n", modelComponent.name.c_str()); fflush(stderr);
                ndShape *shape = shapeFuture.get();
                fprintf(stderr, "[loadShape] .get() returned shape=%s for: %s\n", shape ? "OK" : "null", modelComponent.name.c_str()); fflush(stderr);
                return shape;
            }

            // concurrency::critical_section criticalSection;
            void addEntity(Plugin::Entity *const entity)
            {
                getContext()->log(Context::Info, "Adding entity to physics processor");
                fprintf(stderr, "[addEntity] ENTRY hasTransform=%d hasModel=%d hasScene=%d hasPhysical=%d\n",
                    (int)entity->hasComponent<Components::Transform>(),
                    (int)entity->hasComponent<Components::Model>(),
                    (int)entity->hasComponent<Components::Scene>(),
                    (int)entity->hasComponent<Components::Physical>()); fflush(stderr);

                BodyPtr body;
                if (entity->hasComponent<Components::Transform>())
                {
                    // Handle static scene geometry (Model + Scene, no Physical)
                    if (entity->hasComponents<Components::Model, Components::Scene>() && !entity->hasComponent<Components::Physical>())
                    {
                        auto const &modelComponent = entity->getComponent<Components::Model>();
                        fprintf(stderr, "[addEntity] static-branch: loadShape(%s)\n", modelComponent.name.c_str()); fflush(stderr);
                        auto shape = loadShape(modelComponent);
                        fprintf(stderr, "[addEntity] static-branch: loadShape returned shape=%s\n", shape ? "OK" : "null"); fflush(stderr);
                        if (shape)
                        {
                            auto &transformComponent = entity->getComponent<Components::Transform>();
                            fprintf(stderr, "[addEntity] static-branch: creating StaticBody\n"); fflush(stderr);
                            auto staticBody = std::make_unique<StaticBody>(transformComponent.getMatrix(), ndShapeInstance(shape));
                            fprintf(stderr, "[addEntity] static-branch: StaticBody created\n"); fflush(stderr);
                            if (newtonWorld)
                            {
                                fprintf(stderr, "[addEntity] static-branch: AddBody\n"); fflush(stderr);
                                newtonWorld->AddBody(staticBody->getAsNewtonBody());
                                fprintf(stderr, "[addEntity] static-branch: AddBody done\n"); fflush(stderr);
                            }
                            entityBodyMap[entity] = staticBody.release();
                            fprintf(stderr, "[addEntity] static-branch: done\n"); fflush(stderr);
                        }
                    }
                    // Handle dynamic/kinematic bodies
                    else if (entity->hasComponents<Components::Physical>())
                    {
                        fprintf(stderr, "[addEntity] physical-branch entered\n"); fflush(stderr);
                        auto &physicalComponent = entity->getComponent<Components::Physical>();
                        if (entity->hasComponent<Components::Player>())
                        {
                            fprintf(stderr, "[addEntity] physical-branch: createPlayerBody\n"); fflush(stderr);
                            body = createPlayerBody(core, population, this, entity);
                            fprintf(stderr, "[addEntity] physical-branch: createPlayerBody done\n"); fflush(stderr);
                        }
                        else if (entity->hasComponent<Components::Model>())
                        {
                            auto const &modelComponent = entity->getComponent<Components::Model>();
                            fprintf(stderr, "[addEntity] physical-branch: loadShape(%s)\n", modelComponent.name.c_str()); fflush(stderr);
                            auto shape = loadShape(modelComponent);
                            fprintf(stderr, "[addEntity] physical-branch: loadShape returned shape=%s\n", shape ? "OK" : "null"); fflush(stderr);
                            if (shape)
                            {
                                fprintf(stderr, "[addEntity] physical-branch: createRigidBody\n"); fflush(stderr);
                                body = createRigidBody(this, entity);
                                fprintf(stderr, "[addEntity] physical-branch: createRigidBody done body=%s\n", body ? "OK" : "null"); fflush(stderr);
                                if (body)
                                {
                                    body->getAsNewtonBody()->GetAsBodyDynamic()->SetCollisionShape(ndShapeInstance(shape));
                                    body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, ndShapeInstance(shape));
                                    fprintf(stderr, "[addEntity] physical-branch: SetMassMatrix done\n"); fflush(stderr);
                                }
                            }
                        }
                        else
                        {
                            fprintf(stderr, "[addEntity] physical-branch: no Model component\n"); fflush(stderr);
                        }
                    }
                }

                if (body)
                {
                    if (newtonWorld)
                    {
                        ndSharedPtr<ndBody> sharedBody(body->getAsNewtonBody());
                        auto &transformComponent = entity->getComponent<Components::Transform>();
                        sharedBody->SetMatrix(transformComponent.getMatrix().data);
                        fprintf(stderr, "[addEntity] dynamic AddBody\n"); fflush(stderr);
                        newtonWorld->AddBody(sharedBody);
                        fprintf(stderr, "[addEntity] dynamic AddBody done\n"); fflush(stderr);
                    }
                    entityBodyMap[entity] = body.release();
                }
                fprintf(stderr, "[addEntity] EXIT\n"); fflush(stderr);
            }

            void removeEntity(Plugin::Entity *const entity)
            {
                auto entitySearch = entityBodyMap.find(entity);
                if (entitySearch != std::end(entityBodyMap))
                {
                    newtonWorld->RemoveBody(entitySearch->second->getAsNewtonBody());
                    entityBodyMap.unsafe_erase(entitySearch);
                }
            }

            // Plugin::Core
            void onInitialized(void)
            {
                core->listProcessors([&](Plugin::Processor *processor) -> void
                                     {
                    auto castCheck = dynamic_cast<Edit::Events *>(processor);
                    if (castCheck)
                    {
                        (events = castCheck)->onModified.connect(this, &Processor::onModified);
                    } });
            }

            void onShutdown(void)
            {
                if (events)
                {
                    events->onModified.disconnect(this, &Processor::onModified);
                }

                renderer->onShowUserInterface.disconnect(this, &Processor::onShowUserInterface);
                population->onReset.disconnect(this, &Processor::onReset);
                population->onEntityCreated.disconnect(this, &Processor::onEntityCreated);
                population->onEntityDestroyed.disconnect(this, &Processor::onEntityDestroyed);
                population->onComponentAdded.disconnect(this, &Processor::onComponentAdded);
                population->onComponentRemoved.disconnect(this, &Processor::onComponentRemoved);
                population->onUpdate[50].disconnect(this, &Processor::onUpdate);

                clear();
            }

            // Plugin::Editor Slots
            void onModified(Plugin::Entity *const entity, Hash type)
            {
                auto bodySearch = entityBodyMap.find(entity);
                if (bodySearch == std::end(entityBodyMap))
                {
                    return;
                }

                auto body = bodySearch->second;
                if (type == Components::Transform::GetIdentifier())
                {
                    auto entitySearch = entityBodyMap.find(entity);
                    if (entitySearch != std::end(entityBodyMap))
                    {
                        auto const &transformComponent = entity->getComponent<Components::Transform>();
                        auto matrix(transformComponent.getScaledMatrix());
                        body->getAsNewtonBody()->SetMatrix(matrix.data);
                    }
                }
                else if (type == Components::Model::GetIdentifier())
                {
                    if (!entity->hasComponent<Components::Physical>())
                    {
                        auto const &physicalComponent = entity->getComponent<Components::Physical>();
                        auto const &modelComponent = entity->getComponent<Components::Model>();
                        auto shape = loadShape(modelComponent);
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetCollisionShape(ndShapeInstance(shape));
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, ndShapeInstance(shape));
                    }
                }
                else if (type == Components::Physical::GetIdentifier())
                {
                    if (!entity->hasComponent<Components::Model>())
                    {
                        auto const &physicalComponent = entity->getComponent<Components::Physical>();
                        auto &shapeInstance = body->getAsNewtonBody()->GetAsBodyDynamic()->GetCollisionShape();
                        body->getAsNewtonBody()->GetAsBodyDynamic()->SetMassMatrix(physicalComponent.mass, shapeInstance);
                    }
                }
            }

            // Plugin::Core Slots
            void onShowUserInterface(void)
            {
            }

            // Plugin::Population Slots
            void onReset(void)
            {
                clear();
                newtonWorld = new NewtonWorld(this);

                newtonWorld->Sync();
                newtonWorld->SetSubSteps(2);
                newtonWorld->SetSolverIterations(1);
                newtonWorld->SetThreadCount(4);
                // newtonWorld->SelectSolver(m_solverMode);
            }

            void onEntityCreated(Plugin::Entity *const entity)
            {
                addEntity(entity);
            }

            void onEntityDestroyed(Plugin::Entity *const entity)
            {
                removeEntity(entity);
            }

            void onComponentAdded(Plugin::Entity *const entity)
            {
                addEntity(entity);
            }

            void onComponentRemoved(Plugin::Entity *const entity)
            {
                if (!entity->hasComponents<Components::Transform, Components::Physical>())
                {
                    removeEntity(entity);
                }
            }

            void onUpdate(float frameTime)
            {
                bool editorActive = core->getOption("editor", "active", false);
                if (frameTime > 0.0f && !editorActive)
                {
                    static constexpr float StepTime = (1.0f / 60.0f);
                    while (frameTime > 0.0f)
                    {
                        if (newtonWorld)
                        {
                            newtonWorld->Update(StepTime);
                            newtonWorld->Sync();
                        }

                        frameTime -= StepTime;
                    };
                }
            }

            // Newton::World
            Math::Float3 getGravity(Math::Float3 const *position)
            {
                const Math::Float3 DefaultGravity(0.0f, -32.174f, 0.0f);

                auto localGravity = DefaultGravity;
                if (position)
                {
                }

                return localGravity;
            }

            uint32_t loadSurface(std::string const &surfaceName)
            {
                uint32_t surfaceIndex = 0;

                auto hash = GetHash(surfaceName);
                auto surfaceSearch = surfaceIndexMap.find(hash);
                if (surfaceSearch != std::end(surfaceIndexMap))
                {
                    surfaceIndex = surfaceSearch->second;
                }
                else
                {
                    surfaceIndexMap[hash] = 0;

                    JSON::Object materialNode = JSON::Load(getContext()->findDataPath(FileSystem::CreatePath("materials", surfaceName).withExtension(".json")));
                    auto surfaceNode = materialNode["surface"];
                    if (surfaceNode.is_object())
                    {
                        Surface surface;
                        surface.ghost = JSON::Value(surfaceNode, "ghost", surface.ghost);
                        surface.staticFriction = JSON::Value(surfaceNode, "static_friction", surface.staticFriction);
                        surface.kineticFriction = JSON::Value(surfaceNode, "kinetic_friction", surface.kineticFriction);
                        surface.elasticity = JSON::Value(surfaceNode, "elasticity", surface.elasticity);
                        surface.softness = JSON::Value(surfaceNode, "softness", surface.softness);

                        surfaceIndex = static_cast<uint32_t>(surfaceList.size());
                        surfaceList.push_back(surface);
                        surfaceIndexMap[hash] = surfaceIndex;
                    }
                }

                return surfaceIndex;
            }

            const Surface &getSurface(uint32_t surfaceIndex) const
            {
                static const Surface DefaultSurface;
                return (surfaceIndex >= surfaceList.size() ? DefaultSurface : surfaceList[surfaceIndex]);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Processor)
    }; // namespace Physics
}; // namespace Gek
