#include "API/Engine/ComponentMixin.hpp"
#include "API/Engine/Core.hpp"
#include "API/Engine/Population.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Physics/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/String.hpp"
#include <algorithm>
#include <array>
#include <memory>

namespace Gek
{
    namespace Physics
    {
        class PlayerBody;

        GEK_INTERFACE(State)
        {
            virtual ~State(void) = default;

            virtual void onEnter(PlayerBody * player){};
            virtual void onExit(PlayerBody * player){};

            virtual StatePtr onUpdate(PlayerBody * player, float frameTime) { return nullptr; };
            virtual StatePtr onAction(PlayerBody * player, Plugin::Population::Action const &action) { return nullptr; };
        };

        class IdleState
            : public State
        {
          public:
            StatePtr onUpdate(PlayerBody *player, float frameTime);
            StatePtr onAction(PlayerBody *player, Plugin::Population::Action const &action);
        };

        class WalkingState
            : public State
        {
          public:
            StatePtr onUpdate(PlayerBody *player, float frameTime);
            StatePtr onAction(PlayerBody *player, Plugin::Population::Action const &action);
        };

        class JumpingState
            : public State
        {
          public:
            void onEnter(PlayerBody *player);
            StatePtr onUpdate(PlayerBody *player, float frameTime);
            StatePtr onAction(PlayerBody *player, Plugin::Population::Action const &action);
        };

        class PlayerBody
            : public Body,
              public ndBodyKinematic
        {
          public:
            Plugin::Core *core = nullptr;
            Plugin::Population *population = nullptr;

            Plugin::Entity *const entity = nullptr;

            StatePtr currentState;
            bool moveForward = false;
            bool moveBackward = false;
            bool strafeLeft = false;
            bool strafeRight = false;
            float headingAngle = 0.0f;
            float lookingAngle = 0.0f;
            float forwardSpeed = 0.0f;
            float lateralSpeed = 0.0f;
            float verticalSpeed = 0.0f;

            Math::Float3 groundNormal = Math::Float3::Zero;
            Math::Float3 groundVelocity = Math::Float3::Zero;

            bool touchingSurface = false;

            class NotifyCallback : public ndBodyNotify
            {
              private:
                PlayerBody *playerBody = nullptr;
                World *world = nullptr;

              public:
                NotifyCallback(PlayerBody *playerBody, World *world)
                    : ndBodyNotify(ndVector(world->getGravity().x, world->getGravity().y, world->getGravity().z, 0.0f)), playerBody(playerBody), world(world) {}
                ~NotifyCallback() {}

                void OnTransform(ndFloat32 timestep, const ndMatrix &matrix) override
                {
                    // Update entity transform from physics
                    auto mat = reinterpret_cast<const Math::Float4x4 *>(&matrix);
                    auto &transformComponent = playerBody->entity->getComponent<Components::Transform>();
                    transformComponent.rotation = mat->getRotation();
                    transformComponent.position = mat->translation();
                }

                void OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timeStep) override
                {
                    // Apply gravity/forces if needed
                    auto &physicalComponent = playerBody->entity->getComponent<Components::Physical>();
                    auto &transformComponent = playerBody->entity->getComponent<Components::Transform>();
                    if (playerBody->GetInvMass() > 0.0f)
                    {
                        Math::Float3 gravity(world->getGravity(&transformComponent.position));
                        Math::Float3 force(gravity * physicalComponent.mass);
                        playerBody->SetForce(force.data);
                        playerBody->SetTorque(Math::Float3::Zero.data);
                    }
                }
            };

            ndBodyKinematic *newtonBody = nullptr;

          public:
            PlayerBody(Plugin::Core *core,
                       Plugin::Population *population,
                       World *world,
                       Plugin::Entity *const entity)
                : core(core), population(population), entity(entity), currentState(std::make_unique<IdleState>())
            {
                std::cout << "Creating PlayerBody for entity " << entity << std::endl;
                auto const &transformComponent = entity->getComponent<Components::Transform>();
                auto const &physicalComponent = entity->getComponent<Components::Physical>();
                auto const &playerComponent = entity->getComponent<Components::Player>();

                headingAngle = transformComponent.rotation.getEuler().y;

                // Create Newton kinematic body for player
                fprintf(stderr, "[addEntity] Setting up Newton body for player entity %p\n", entity);
                fflush(stderr);
                SetNotifyCallback(new NotifyCallback(this, world));
                fprintf(stderr, "Getting matrix for player entity %p\n", entity);
                fflush(stderr);
                ndMatrix matrix(transformComponent.getMatrix().data);
                fprintf(stderr, "Setting matrix for player entity %p, matrix: %p\n", entity, &matrix);
                fflush(stderr);
                SetMatrix(matrix);
                fprintf(stderr, "Setting mass matrix for player entity %p\n", entity);
                fflush(stderr);
                SetMassMatrix(physicalComponent.mass, ndShapeInstance(new ndShapeCapsule(playerComponent.innerRadius, playerComponent.outerRadius, playerComponent.height)));
                fprintf(stderr, "Disabling auto-sleep for player entity %p\n", entity);
                fflush(stderr);
                SetAutoSleep(false);

                fprintf(stderr, "PlayerBody constructor finished for entity %p\n", entity);
                fflush(stderr);
                newtonBody = this;

                fprintf(stderr, "Connecting PlayerBody to population signals for entity %p\n", entity);
                fflush(stderr);
                population->onAction.connect(this, &PlayerBody::onAction);
                fprintf(stderr, "PlayerBody setup complete for entity %p\n", entity);
                fflush(stderr);
            }

            ~PlayerBody(void)
            {
                population->onAction.disconnect(this, &PlayerBody::onAction);
            }

            // Plugin::Population Slots
            void onAction(Plugin::Population::Action const &action)
            {
                bool editorActive = core->getOption("editor", "active", false);
                if (editorActive)
                {
                    return;
                }

                if (action.name == "turn")
                {
                    headingAngle += (action.value * 0.01f);
                }
                else if (action.name == "tilt")
                {
                    lookingAngle += (action.value * 0.01f);
                    lookingAngle = Math::Clamp(lookingAngle, -Math::Pi * 0.5f, Math::Pi * 0.5f);
                }
                else if (action.name == "move_forward")
                {
                    moveForward = action.state;
                }
                else if (action.name == "move_backward")
                {
                    moveBackward = action.state;
                }
                else if (action.name == "strafe_left")
                {
                    strafeLeft = action.state;
                }
                else if (action.name == "strafe_right")
                {
                    strafeRight = action.state;
                }
                else if (action.name == "crouch")
                {
                }

                StatePtr nextState(currentState->onAction(this, action));
                if (nextState)
                {
                    currentState->onExit(this);
                    nextState->onEnter(this);
                    currentState = std::move(nextState);
                }
            }

            // Body
            ndBody *getAsNewtonBody(void) override
            {
                return newtonBody;
            }
        };

        StatePtr IdleState::onUpdate(PlayerBody *player, float frameTime)
        {
            return nullptr;
        }

        StatePtr IdleState::onAction(PlayerBody *player, Plugin::Population::Action const &action)
        {
            if (action.name == "crouch" && action.state)
            {
            }
            else if (action.name == "move_forward" && action.state)
            {
                return std::make_unique<WalkingState>();
            }
            else if (action.name == "move_backward" && action.state)
            {
                return std::make_unique<WalkingState>();
            }
            else if (action.name == "strafe_left" && action.state)
            {
                return std::make_unique<WalkingState>();
            }
            else if (action.name == "strafe_right" && action.state)
            {
                return std::make_unique<WalkingState>();
            }
            else if (action.name == "jump" && action.state && player->touchingSurface)
            {
                return std::make_unique<JumpingState>();
            }

            return nullptr;
        }

        StatePtr WalkingState::onUpdate(PlayerBody *player, float frameTime)
        {
            if (!player->moveForward && !player->moveBackward && !player->strafeLeft && !player->strafeRight)
            {
                return std::make_unique<IdleState>();
            }

            player->forwardSpeed += (((player->moveForward ? 1.0f : 0.0f) + (player->moveBackward ? -1.0f : 0.0f)) * 5.0f);
            player->lateralSpeed += (((player->strafeLeft ? -1.0f : 0.0f) + (player->strafeRight ? 1.0f : 0.0f)) * 5.0f);
            return nullptr;
        }

        StatePtr WalkingState::onAction(PlayerBody *player, Plugin::Population::Action const &action)
        {
            if (action.name == "jump" && action.state && player->touchingSurface)
            {
                return std::make_unique<JumpingState>();
            }

            return nullptr;
        }

        void JumpingState::onEnter(PlayerBody *player)
        {
            player->verticalSpeed += 20.0f;
            player->touchingSurface = false;
        }

        StatePtr JumpingState::onUpdate(PlayerBody *player, float frameTime)
        {
            if (player->touchingSurface)
            {
                if (player->moveForward || player->moveBackward || player->strafeLeft || player->strafeRight)
                {
                    return std::make_unique<WalkingState>();
                }
                else
                {
                    return std::make_unique<IdleState>();
                }
            }

            return nullptr;
        }

        StatePtr JumpingState::onAction(PlayerBody *player, Plugin::Population::Action const &action)
        {
            if (action.name == "jump" && action.state && player->touchingSurface)
            {
                return std::make_unique<JumpingState>();
            }

            return nullptr;
        }

        BodyPtr createPlayerBody(Plugin::Core *core, Plugin::Population *population, World *world, Plugin::Entity *const entity)
        {
            return std::make_unique<PlayerBody>(core, population, world, entity);
        }
    }; // namespace Physics
}; // namespace Gek