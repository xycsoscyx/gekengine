#include "API/Engine/ComponentMixin.hpp"
#include "API/Engine/Core.hpp"
#include "API/Engine/Population.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Physics/Base.hpp"
#include "GEK/Physics/MatrixUtil.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/String.hpp"
#include <algorithm>
#include <array>
#include <cmath>
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
            void onEnter(PlayerBody *player);
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
              public ndBodyDynamic
        {
          public:
            Plugin::Core *core = nullptr;
            Plugin::Population *population = nullptr;
            World *world = nullptr;

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
            float groundedGraceTime = 0.0f;

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
                    transformComponent.rotation = Math::Quaternion::MakeEulerRotation(playerBody->lookingAngle, playerBody->headingAngle, 0.0f);
                    transformComponent.position = mat->translation();
                }

                void OnApplyExternalForce(ndInt32 threadIndex, ndFloat32 timeStep) override
                {
                    // Apply gravity and movement intent as bounded force.
                    auto &physicalComponent = playerBody->entity->getComponent<Components::Physical>();
                    auto &transformComponent = playerBody->entity->getComponent<Components::Transform>();
                    if (playerBody->GetInvMass() > 0.0f)
                    {
                        Math::Float3 gravity(world->getGravity(&transformComponent.position));
                        Math::Float3 force(gravity * physicalComponent.mass);

                        float const sinHeading = std::sin(playerBody->headingAngle);
                        float const cosHeading = std::cos(playerBody->headingAngle);
                        Math::Float3 const forwardDirection(sinHeading, 0.0f, cosHeading);
                        Math::Float3 const rightDirection(forwardDirection.z, 0.0f, -forwardDirection.x);

                        Math::Float3 const desiredVelocity((forwardDirection * playerBody->forwardSpeed) +
                                                           (rightDirection * playerBody->lateralSpeed));

                        float const controlForceScale = playerBody->touchingSurface ? 24.0f : 8.0f;
                        force += (desiredVelocity * (physicalComponent.mass * controlForceScale));

                        if (playerBody->verticalSpeed > 0.0f)
                        {
                            float const safeStep = std::max(timeStep, 0.001f);
                            force.y += (physicalComponent.mass * (playerBody->verticalSpeed / safeStep));
                            playerBody->verticalSpeed = 0.0f;
                        }

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
                : core(core), population(population), world(world), entity(entity), currentState(std::make_unique<IdleState>())
            {
                std::cout << "Creating PlayerBody for entity " << entity << std::endl;
                auto const &transformComponent = entity->getComponent<Components::Transform>();
                auto const &physicalComponent = entity->getComponent<Components::Physical>();
                auto const &playerComponent = entity->getComponent<Components::Player>();

                headingAngle = transformComponent.rotation.getEuler().y;

                // Create Newton dynamic body for player
                SetNotifyCallback(new NotifyCallback(this, world));
                auto matrix(transformComponent.getMatrix());
                SetMatrix(MakeNewtonMatrix(matrix));

                ndShapeInstance capsuleShape(new ndShapeCapsule(playerComponent.innerRadius, playerComponent.outerRadius, playerComponent.height));
                SetCollisionShape(capsuleShape);
                SetMassMatrix(physicalComponent.mass, capsuleShape);
                SetLinearDamping(0.3f);
                SetAutoSleep(false);

                newtonBody = this;

                population->onAction.connect(this, &PlayerBody::onAction);
                population->onUpdate[49].connect(this, &PlayerBody::onUpdate);
                world->onCollision.connect(this, &PlayerBody::onCollision);
            }

            ~PlayerBody(void)
            {
                world->onCollision.disconnect(this, &PlayerBody::onCollision);
                population->onUpdate[49].disconnect(this, &PlayerBody::onUpdate);
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

            void onUpdate(float frameTime)
            {
                if (frameTime <= 0.0f)
                {
                    return;
                }

                touchingSurface = (groundedGraceTime > 0.0f);

                StatePtr nextState(currentState->onUpdate(this, frameTime));
                if (nextState)
                {
                    currentState->onExit(this);
                    nextState->onEnter(this);
                    currentState = std::move(nextState);
                }

                groundedGraceTime = std::max(0.0f, groundedGraceTime - frameTime);

                if (std::abs(forwardSpeed) < 0.05f)
                {
                    forwardSpeed = 0.0f;
                }

                if (std::abs(lateralSpeed) < 0.05f)
                {
                    lateralSpeed = 0.0f;
                }
            }

            void onCollision(Plugin::Entity *entity0, Math::Float3 const &, Math::Float3 const &normal, Plugin::Entity *entity1)
            {
                if (entity0 == entity)
                {
                    if (normal.y > 0.25f)
                    {
                        touchingSurface = true;
                        groundedGraceTime = 0.35f;
                        groundNormal = normal;
                    }
                }
                else if (entity1 == entity)
                {
                    Math::Float3 const adjustedNormal(normal * -1.0f);
                    if (adjustedNormal.y > 0.25f)
                    {
                        touchingSurface = true;
                        groundedGraceTime = 0.35f;
                        groundNormal = adjustedNormal;
                    }
                }
            }

            // Body
            ndBody *getAsNewtonBody(void) override
            {
                return newtonBody;
            }
        };

        void IdleState::onEnter(PlayerBody *player)
        {
            player->forwardSpeed = 0.0f;
            player->lateralSpeed = 0.0f;
        }

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

            float const targetForwardSpeed = (((player->moveForward ? 1.0f : 0.0f) + (player->moveBackward ? -1.0f : 0.0f)) * 8.0f);
            float const targetLateralSpeed = (((player->strafeLeft ? -1.0f : 0.0f) + (player->strafeRight ? 1.0f : 0.0f)) * 8.0f);
            float const blend = Math::Clamp(frameTime * 12.0f, 0.0f, 1.0f);

            player->forwardSpeed = Math::Interpolate(player->forwardSpeed, targetForwardSpeed, blend);
            player->lateralSpeed = Math::Interpolate(player->lateralSpeed, targetLateralSpeed, blend);
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