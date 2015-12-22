#include "GEK\Newton\PlayerBody.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Engine\Action.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\NewtonEntity.h"
#include "GEK\Newton\PlayerBody.h"
#include "GEK\Newton\Mass.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Matrix4x4.h"
#include <Newton.h>
#include <dNewton.h>
#include <dNewtonPlayerManager.h>
#include <dNewtonCollision.h>

namespace Gek
{
    static const Math::Float3 Gravity(0.0f, -32.174f, 0.0f);

    DECLARE_INTERFACE(State)
    {
        STDMETHOD_(void, onEnter)       (THIS) { };
        STDMETHOD_(void, onExit)        (THIS) { };

        STDMETHOD_(State *, onUpdate)   (THIS_ float frameTime) { return nullptr; };
        STDMETHOD_(State *, onAction)   (THIS_ LPCWSTR name, const ActionParam &param) { return nullptr; };
    };

    class PlayerNewtonBody;
    State *createIdleState(PlayerNewtonBody *playerBody);
    State *createCrouchgingState(PlayerNewtonBody *playerBody);
    State *createWalkingState(PlayerNewtonBody *playerBody, LPCWSTR trigger);
    State *createJumpingState(PlayerNewtonBody *playerBody);
    State *createJumpedState(PlayerNewtonBody *playerBody);

    class PlayerNewtonBody : public UnknownMixin
        , virtual public ActionObserver
        , virtual public NewtonEntity
    {
    private:
        IUnknown *actionProvider;
        CustomPlayerControllerManager *newtonPlayerManager;
        CustomPlayerController *newtonPlayerController;
        Entity *entity;
        State *currentState;
        float heading;

    public:
        PlayerNewtonBody(IUnknown *actionProvider, CustomPlayerControllerManager *newtonPlayerManager, CustomPlayerController *newtonPlayerController, Entity *entity,
            const PlayerBodyComponent &playerBodyComponent,
            const TransformComponent &transformComponent,
            const MassComponent &massComponent)
            : actionProvider(actionProvider)
            , newtonPlayerManager(newtonPlayerManager)
            , newtonPlayerController(newtonPlayerController)
            , entity(entity)
            , currentState(createIdleState(this))
            , heading(0.0f)
        {
            REQUIRE_VOID_RETURN(newtonPlayerController);

            NewtonBodySetUserData(newtonPlayerController->GetBody(), this);
            NewtonBodySetMatrix(newtonPlayerController->GetBody(), Math::Float4x4(transformComponent.rotation, transformComponent.position).data);
        }

        ~PlayerNewtonBody(void)
        {
            REQUIRE_VOID_RETURN(newtonPlayerManager);
            REQUIRE_VOID_RETURN(actionProvider);

            newtonPlayerManager->DestroyController(newtonPlayerController);
            ObservableMixin::removeObserver(actionProvider, getClass<ActionObserver>());
        }

        // IUnknown
        BEGIN_INTERFACE_LIST(PlayerNewtonBody)
            INTERFACE_LIST_ENTRY_COM(ActionObserver);
        END_INTERFACE_LIST_UNKNOWN

        // NewtonEntity
        STDMETHODIMP_(Entity *) getEntity(void) const
        {
            return entity;
        }

        STDMETHODIMP_(NewtonBody *) getNewtonBody(void) const
        {
            return newtonPlayerController->GetBody();
        }

        // PlayerBody
        void onUpdate(float frameTime)
        {
            State *newState = (currentState ? currentState->onUpdate(frameTime) : nullptr);
            if (newState)
            {
                currentState->onExit();
                delete currentState;
                currentState = newState;
                newState->onEnter();
            }
        }

        bool isInFreeFall(void)
        {
            REQUIRE_RETURN(newtonPlayerController, false);

            return newtonPlayerController->IsInFreeFall();
        }

        void setPlayerVelocity(dFloat forwardSpeed, dFloat lateralSpeed, dFloat verticalSpeed, dFloat timeStep)
        {
            REQUIRE_VOID_RETURN(newtonPlayerController);

            newtonPlayerController->SetPlayerVelocity(forwardSpeed, lateralSpeed, verticalSpeed, heading, Gravity.data, timeStep);
        }

        // ActionObserver
        STDMETHODIMP_(void) onAction(LPCWSTR name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"turn") == 0)
            {
                heading += (param.value * 0.01f);
            }

            State *newState = (currentState ? currentState->onAction(name, param) : nullptr);
            if (newState)
            {
                currentState->onExit();
                delete currentState;
                currentState = newState;
                newState->onEnter();
            }
        }
    };

    class PlayerStateMixin : public State
    {
    protected:
        PlayerNewtonBody *playerBody;

    public:
        PlayerStateMixin(PlayerNewtonBody *playerBody)
            : playerBody(playerBody)
        {
        }
    };

    class IdleState : public PlayerStateMixin
    {
    public:
        IdleState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onAction(LPCWSTR name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"crouch") == 0 && param.state)
            {
                return createCrouchgingState(playerBody);
            }
            else if (_wcsicmp(name, L"move_forward") == 0 && param.state)
            {
                return createWalkingState(playerBody, name);
            }
            else if (_wcsicmp(name, L"move_backward") == 0 && param.state)
            {
                return createWalkingState(playerBody, name);
            }
            else if (_wcsicmp(name, L"strafe_left") == 0 && param.state)
            {
                return createWalkingState(playerBody, name);
            }
            else if (_wcsicmp(name, L"strafe_right") == 0 && param.state)
            {
                return createWalkingState(playerBody, name);
            }
            else if (_wcsicmp(name, L"jump") == 0 && param.state)
            {
                return createJumpingState(playerBody);
            }

            return nullptr;
        }
    };

    class CrouchingState : public PlayerStateMixin
    {
    public:
        CrouchingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onAction(LPCWSTR name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"crouch") == 0 && !param.state)
            {
                return createIdleState(playerBody);
            }

            return nullptr;
        }
    };

    class WalkingState : public PlayerStateMixin
    {
    private:
        bool moveForward;
        bool moveBackward;
        bool strafeLeft;
        bool strafeRight;

    public:
        WalkingState(PlayerNewtonBody *playerBody, LPCWSTR trigger)
            : PlayerStateMixin(playerBody)
            , moveForward(false)
            , moveBackward(false)
            , strafeLeft(false)
            , strafeRight(false)
        {
            onAction(trigger, true);
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            REQUIRE_RETURN(playerBody, nullptr);

            float lateralSpeed = ((moveForward ? -1.0f : 0.0f) + (moveBackward ? 1.0f : 0.0f)) * 5.0f;
            float strafeSpeed = ((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f;
            playerBody->setPlayerVelocity(lateralSpeed, strafeSpeed, 0.0f, frameTime);
            return nullptr;
        }

        STDMETHODIMP_(State *) onAction(LPCWSTR name, const ActionParam &param)
        {
            if (_wcsicmp(name, L"move_forward") == 0)
            {
                moveForward = param.state;
            }
            else if (_wcsicmp(name, L"move_backward") == 0)
            {
                moveBackward = param.state;
            }
            else if (_wcsicmp(name, L"strafe_left") == 0)
            {
                strafeLeft = param.state;
            }
            else if (_wcsicmp(name, L"strafe_right") == 0)
            {
                strafeRight = param.state;
            }
            else if (_wcsicmp(name, L"jump") == 0 && param.state)
            {
                return createJumpingState(playerBody);
            }

            if (!moveForward &&
                !moveBackward &&
                !strafeLeft &&
                !strafeRight)
            {
                return createIdleState(playerBody);
            }

            return nullptr;
        }
    };

    class JumpingState : public PlayerStateMixin
    {
    public:
        JumpingState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            REQUIRE_RETURN(playerBody, nullptr);

            playerBody->setPlayerVelocity(0.0f, 0.0f, 50.0f, frameTime);

            return createJumpedState(playerBody);
        }
    };

    class JumpedState : public PlayerStateMixin
    {
    public:
        JumpedState(PlayerNewtonBody *playerBody)
            : PlayerStateMixin(playerBody)
        {
        }

        // State
        STDMETHODIMP_(State *) onUpdate(float frameTime)
        {
            REQUIRE_RETURN(playerBody, nullptr);

            if (playerBody->isInFreeFall())
            {
                return createIdleState(playerBody);
            }

            return nullptr;
        }
    };

    State *createIdleState(PlayerNewtonBody *playerBody)
    {
        return new IdleState(playerBody);
    }

    State *createCrouchgingState(PlayerNewtonBody *playerBody)
    {
        return new IdleState(playerBody);
    }

    State *createWalkingState(PlayerNewtonBody *playerBody, LPCWSTR trigger)
    {
        return new WalkingState(playerBody, trigger);
    }

    State *createJumpingState(PlayerNewtonBody *playerBody)
    {
        return new JumpingState(playerBody);
    }

    State *createJumpedState(PlayerNewtonBody *playerBody)
    {
        return new JumpedState(playerBody);
    }

    class NewtonPlayerControllerManager : public CustomPlayerControllerManager
    {
    public:
        NewtonPlayerControllerManager(NewtonWorld* const world)
            : CustomPlayerControllerManager(world)
        {
        }

        ~NewtonPlayerControllerManager()
        {
        }

        // CustomPlayerControllerManager
        void ApplyPlayerMove(CustomPlayerController* const controller, dFloat frameTime)
        {
            //PlayerNewtonBody *playerBody = static_cast<PlayerNewtonBody *>(controller->GetUserData());
            //playerBody->onUpdate(frameTime);
        }
    };

    CustomPlayerControllerManager *createPlayerManager(NewtonWorld *newton)
    {
        return new NewtonPlayerControllerManager(newton);
    }

    NewtonEntity *createPlayerBody(IUnknown *actionProvider, CustomPlayerControllerManager *newtonPlayerManager, Entity *entity, const PlayerBodyComponent &playerBodyComponent, const TransformComponent &transformComponent, const MassComponent &massComponent)
    {
        dMatrix playerAxis;
        playerAxis[0] = dVector(0.0f, 1.0f, 0.0f, 0.0f);
        playerAxis[1] = dVector(1.0f, 0.0f, 0.0f, 0.0f);
        playerAxis[2] = dVector(0.0f, 0.0f, -1.0f, 0.0f);
        playerAxis[3] = dVector(0.0f, 0.0f, 0.0f, 1.0f);
        PlayerNewtonBody *playerBody = new PlayerNewtonBody(actionProvider, newtonPlayerManager, 
            newtonPlayerManager->CreatePlayer(massComponent, playerBodyComponent.outerRadius, playerBodyComponent.innerRadius, playerBodyComponent.height, playerBodyComponent.stairStep, playerAxis),
            entity, playerBodyComponent, transformComponent, massComponent);
        ObservableMixin::addObserver(actionProvider, playerBody->getClass<ActionObserver>());
        return playerBody;
    }

    PlayerBodyComponent::PlayerBodyComponent(void)
        : outerRadius(1.0f)
        , innerRadius(0.25f)
        , height(1.9f)
        , stairStep(0.25f)
    {
    }

    HRESULT PlayerBodyComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L"outer_radius"] = String::from(outerRadius);
        componentParameterList[L"inner_radius"] = String::from(innerRadius);
        componentParameterList[L"height"] = String::from(height);
        componentParameterList[L"stair_step"] = String::from(stairStep);
        return S_OK;
    }

    HRESULT PlayerBodyComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"outer_radius", outerRadius, String::to<float>);
        setParameter(componentParameterList, L"inner_radius", innerRadius, String::to<float>);
        setParameter(componentParameterList, L"height", height, String::to<float>);
        setParameter(componentParameterList, L"stair_step", stairStep, String::to<float>);
        return S_OK;
    }

    class PlayerBodyImplementation : public ContextUserMixin
        , public ComponentMixin<PlayerBodyComponent>
    {
    public:
        PlayerBodyImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(PlayerBodyImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
            END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"player_body";
        }
    };

    REGISTER_CLASS(PlayerBodyImplementation)
}; // namespace Gek