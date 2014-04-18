#include "CGEKComponentScript.h"
#include <luabind/adopt_policy.hpp>
#include <algorithm>
#include <string>

#pragma comment (lib, "lua5.1.lib")

#ifdef _DEBUG
    #pragma comment (lib, "luabindd.lib")
#else
    #pragma comment (lib, "luabind.lib")
#endif

luabind::object GetObjectFromValue(lua_State *pState, const GEKVALUE &kValue)
{
    switch (kValue.GetType())
    {
    case GEKVALUE::BOOLEAN:
        return luabind::object(pState, kValue.GetBoolean());

    case GEKVALUE::INTEGER:
        return luabind::object(pState, kValue.GetUINT32());

    case GEKVALUE::FLOAT:
        return luabind::object(pState, kValue.GetFloat());

    case GEKVALUE::FLOAT2:
        return luabind::object(pState, kValue.GetFloat2());

    case GEKVALUE::FLOAT3:
        return luabind::object(pState, kValue.GetFloat3());

    case GEKVALUE::FLOAT4:
        return luabind::object(pState, kValue.GetFloat4());

    case GEKVALUE::QUATERNION:
        return luabind::object(pState, kValue.GetQuaternion());

    case GEKVALUE::STRING:
        return luabind::object(pState, (LPCSTR)CW2A(kValue.GetString(), CP_UTF8));
    };

    return luabind::object();
}

int OutputLuaError(lua_State *pState)
{
    luabind::object kMessage(luabind::from_stack(pState, -1));
    if (kMessage.is_valid())
    {
        OutputDebugStringA(FormatString("Lua Runtime Error: %s\r\n", luabind::object_cast<std::string>(kMessage).c_str()));
        std::string strTraceBack = luabind::call_function<std::string>(luabind::globals(pState)["debug"]["traceback"]);
        if (strTraceBack.length() > 0)
        {
            OutputDebugStringA(FormatString("%s\r\n", strTraceBack.c_str()));
        }
    }
    else
    {
        OutputDebugStringA("Unknown Lua Runtime Error\r\n");
    }

    return 1;
}

BEGIN_INTERFACE_LIST(CGEKComponentScript)
    INTERFACE_LIST_ENTRY_COM(IGEKComponent)
    INTERFACE_LIST_ENTRY_COM(IGEKViewManagerUser)
END_INTERFACE_LIST_UNKNOWN

CGEKComponentScript::CGEKComponentScript(IGEKEntity *pEntity, lua_State *pState)
    : CGEKComponent(pEntity)
    , m_pState(pState)
    , m_bOnRender(false)
{
}

CGEKComponentScript::~CGEKComponentScript(void)
{
}

bool CGEKComponentScript::HasOnRender(void)
{
    return m_bOnRender;
}

void CGEKComponentScript::SetCurrentState(const luabind::object &kState)
{
    if (m_kCurrentState.is_valid())
    {
        m_kCurrentState["OnExit"](GetEntity());
    }

    std::map<CStringA, bool> aRequiredFunctions;
    aRequiredFunctions["OnEnter"] = false;
    aRequiredFunctions["OnExit"] = false;
    aRequiredFunctions["OnUpdate"] = false;
    aRequiredFunctions["OnEvent"] = false;
    for (luabind::iterator pIterator(kState), pEnd; pIterator != pEnd; ++pIterator)
    {
        const luabind::object &kKey = pIterator.key();
        if (luabind::type(kKey) == LUA_TSTRING)
        {
            CStringA strKey = luabind::object_cast<std::string>(kKey).c_str();
            const luabind::object &kValue = (*pIterator);
            if (kValue.is_valid())
            {
                aRequiredFunctions[strKey] = true;
                if (strKey.CompareNoCase("OnRender") == 0)
                {
                    m_bOnRender = true;
                }
            }
        }
    }

    bool bValidState = true;
    for (auto &kPair : aRequiredFunctions)
    {
        if (!kPair.second)
        {
            bValidState = false;
            OutputDebugStringA("Script Error: state missing function: " + kPair.first + "\r\n");
        }
    }

    if (bValidState)
    {
        m_kCurrentState = kState;
        if (m_kCurrentState.is_valid())
        {
            m_kCurrentState["OnEnter"](GetEntity());
        }
    }
}

luabind::object &CGEKComponentScript::GetCurrentState(void)
{
    return m_kCurrentState;
}

STDMETHODIMP_(LPCWSTR) CGEKComponentScript::GetType(void) const
{
    return L"script";
}

STDMETHODIMP_(void) CGEKComponentScript::ListProperties(std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty)
{
    OnProperty(L"source", m_strScript.GetString());
    OnProperty(L"params", m_strParams.GetString());
}

static GEKHASH gs_nSource(L"source");
static GEKHASH gs_nParams(L"params");
STDMETHODIMP_(bool) CGEKComponentScript::GetProperty(LPCWSTR pName, GEKVALUE &kValue) const
{
    GEKHASH nHash(pName);
    if (nHash == gs_nSource)
    {
        kValue = m_strScript.GetString();
        return true;
    }
    else if (nHash == gs_nParams)
    {
        kValue = m_strParams.GetString();
        return true;
    }

    return false;
}

STDMETHODIMP_(bool) CGEKComponentScript::SetProperty(LPCWSTR pName, const GEKVALUE &kValue)
{
    GEKHASH nHash(pName);
    if (nHash == gs_nSource)
    {
        m_strScript = kValue.GetString();
        return true;
    }
    else if (nHash == gs_nParams)
    {
        m_strParams = kValue.GetString();
        return true;
    }

    return false;
}

STDMETHODIMP CGEKComponentScript::OnEntityCreated(void)
{
    HRESULT hRetVal = S_OK;
    if (!m_strScript.IsEmpty())
    {
        CStringA strScript;
        hRetVal = GEKLoadFromFile((L"%root%\\data\\scripts\\" + m_strScript + ".lua"), strScript);
        if (SUCCEEDED(hRetVal))
        {
            if (strScript.IsEmpty())
            {
                hRetVal = E_INVALID;
            }
            else
            {
                luaL_dostring(m_pState, strScript);
                CStringA strParamsUTF8 = CW2A(m_strParams, CP_UTF8);
                luabind::call_function<void>(m_pState, "Initialize", GetEntity(), strParamsUTF8.GetString());
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentScript::OnEvent(LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB)
{
    if (m_kCurrentState.is_valid())
    {
        CStringA strActionUTF8 = CW2A(pAction, CP_UTF8);
        m_kCurrentState["OnEvent"](GetEntity(), strActionUTF8.GetString(), GetObjectFromValue(m_pState, kParamA), GetObjectFromValue(m_pState, kParamB));
    }
}

BEGIN_INTERFACE_LIST(CGEKComponentSystemScript)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemScript)

CGEKComponentSystemScript::CGEKComponentSystemScript(void)
    : m_pState(nullptr)
{
}

CGEKComponentSystemScript::~CGEKComponentSystemScript(void)
{
}

namespace Entity
{
    void EnablePass(IGEKEntity *pEntity, LPCSTR strPass)
    {
        REQUIRE_VOID_RETURN(pEntity);
        CGEKComponentScript *pScript = dynamic_cast<CGEKComponentScript *>(pEntity->GetComponent(L"script"));
        if (pScript)
        {
            pScript->GetViewManager()->EnablePass(CA2W(strPass, CP_UTF8));
        }
    }

    void CaptureMouse(IGEKEntity *pEntity, bool bCapture)
    {
        REQUIRE_VOID_RETURN(pEntity);
        CGEKComponentScript *pScript = dynamic_cast<CGEKComponentScript *>(pEntity->GetComponent(L"script"));
        if (pScript)
        {
            pScript->GetViewManager()->CaptureMouse(bCapture);
        }
    }

    void SetState(IGEKEntity *pEntity, const luabind::object &kState)
    {
        REQUIRE_VOID_RETURN(pEntity);
        CGEKComponentScript *pScript = dynamic_cast<CGEKComponentScript *>(pEntity->GetComponent(L"script"));
        if (pScript)
        {
            pScript->SetCurrentState(kState);
        }
    }

    static luabind::object nNil;
    luabind::object &GetState(IGEKEntity *pEntity)
    {
        REQUIRE_RETURN(pEntity, nNil);
        CGEKComponentScript *pScript = dynamic_cast<CGEKComponentScript *>(pEntity->GetComponent(L"script"));
        if (pScript)
        {
            return pScript->GetCurrentState();
        }

        return nNil;
    }

    luabind::object GetProperty(lua_State *pState, IGEKEntity *pEntity, LPCSTR strComponent, LPCSTR strProperty)
    {
        REQUIRE_RETURN(pEntity, nNil);
        IGEKComponent *pComponent = pEntity->GetComponent((LPWSTR)CA2W(strComponent, CP_UTF8));
        if (pComponent)
        {
            GEKVALUE kValue;
            if (pComponent->GetProperty((LPWSTR)CA2W(strProperty, CP_UTF8), kValue))
            {
                return GetObjectFromValue(pState, kValue);
            }
        }

        return nNil;
    }

    bool SetProperty(IGEKEntity *pEntity, LPCSTR strComponent, LPCSTR strProperty, bool bValue)
    {
        REQUIRE_RETURN(pEntity, false);
        IGEKComponent *pComponent = pEntity->GetComponent((LPWSTR)CA2W(strComponent, CP_UTF8));
        if (pComponent)
        {
            return pComponent->SetProperty((LPWSTR)CA2W(strProperty, CP_UTF8), bValue);
        }

        return false;
    }

    bool SetProperty(IGEKEntity *pEntity, LPCSTR strComponent, LPCSTR strProperty, double nValue)
    {
        REQUIRE_RETURN(pEntity, false);
        IGEKComponent *pComponent = pEntity->GetComponent((LPWSTR)CA2W(strComponent, CP_UTF8));
        if (pComponent)
        {
            return pComponent->SetProperty((LPWSTR)CA2W(strProperty, CP_UTF8), float(nValue));
        }

        return false;
    }

    bool SetProperty(IGEKEntity *pEntity, LPCSTR strComponent, LPCSTR strProperty, float2 nValue)
    {
        REQUIRE_RETURN(pEntity, false);
        IGEKComponent *pComponent = pEntity->GetComponent((LPWSTR)CA2W(strComponent, CP_UTF8));
        if (pComponent)
        {
            return pComponent->SetProperty((LPWSTR)CA2W(strProperty, CP_UTF8), nValue);
        }

        return false;
    }

    bool SetProperty(IGEKEntity *pEntity, LPCSTR strComponent, LPCSTR strProperty, float3 nValue)
    {
        REQUIRE_RETURN(pEntity, false);
        IGEKComponent *pComponent = pEntity->GetComponent((LPWSTR)CA2W(strComponent, CP_UTF8));
        if (pComponent)
        {
            return pComponent->SetProperty((LPWSTR)CA2W(strProperty, CP_UTF8), nValue);
        }

        return false;
    }

    bool SetProperty(IGEKEntity *pEntity, LPCSTR strComponent, LPCSTR strProperty, float4 nValue)
    {
        REQUIRE_RETURN(pEntity, false);
        IGEKComponent *pComponent = pEntity->GetComponent((LPWSTR)CA2W(strComponent, CP_UTF8));
        if (pComponent)
        {
            return pComponent->SetProperty((LPWSTR)CA2W(strProperty, CP_UTF8), nValue);
        }

        return false;
    }

    bool SetProperty(IGEKEntity *pEntity, LPCSTR strComponent, LPCSTR strProperty, quaternion nValue)
    {
        REQUIRE_RETURN(pEntity, false);
        IGEKComponent *pComponent = pEntity->GetComponent((LPWSTR)CA2W(strComponent, CP_UTF8));
        if (pComponent)
        {
            return pComponent->SetProperty((LPWSTR)CA2W(strProperty, CP_UTF8), nValue);
        }

        return false;
    }

    bool SetProperty(IGEKEntity *pEntity, LPCSTR strComponent, LPCSTR strProperty, LPCSTR pValue)
    {
        REQUIRE_RETURN(pEntity, false);
        IGEKComponent *pComponent = pEntity->GetComponent((LPWSTR)CA2W(strComponent, CP_UTF8));
        if (pComponent)
        {
            CStringW strValue = CA2W(pValue, CP_UTF8);
            return pComponent->SetProperty((LPWSTR)CA2W(strProperty, CP_UTF8), strValue.GetString());
        }

        return false;
    }
};
   
namespace String
{
    luabind::object Split(lua_State *pState, LPCSTR strString, LPCSTR strSeparator)
    {
        luabind::object kTable;
        if (strString && strSeparator)
        {
            kTable = luabind::newtable(pState);
            LPCSTR strNext = nullptr;
            int nIndex = 1;

            while (strString && (strNext = strchr(strString, *strSeparator)) != nullptr)
            {
                kTable[nIndex++] = std::string(strString, (strNext - strString));
                strString = (strNext + 1);
            };

            if (strString)
            {
                kTable[nIndex] = std::string(strString);
            }
        }

        return kTable;
    }
};

STDMETHODIMP CGEKComponentSystemScript::Initialize(void)
{
    m_pState = luaL_newstate();
    if (m_pState)
    {
        luabind::open(m_pState);
        luabind::set_pcall_callback(OutputLuaError);
        luaL_openlibs(m_pState);

        luabind::module(m_pState, "string")
        [
            luabind::def("split", String::Split)
        ];

        luabind::module(m_pState)
        [
            luabind::class_<IGEKEntity>("Entity")
            .def("EnablePass", &Entity::EnablePass)
            .def("CaptureMouse", &Entity::CaptureMouse)
            .def("SetState", &Entity::SetState)
            .def("GetState", &Entity::GetState)
            .def("GetProperty", &Entity::GetProperty)
            .def("SetProperty", (bool (*)(IGEKEntity *, LPCSTR , LPCSTR , bool))&Entity::SetProperty)
            .def("SetProperty", (bool (*)(IGEKEntity *, LPCSTR , LPCSTR , double))&Entity::SetProperty)
            .def("SetProperty", (bool (*)(IGEKEntity *, LPCSTR , LPCSTR , float2))&Entity::SetProperty)
            .def("SetProperty", (bool (*)(IGEKEntity *, LPCSTR , LPCSTR , float3))&Entity::SetProperty)
            .def("SetProperty", (bool (*)(IGEKEntity *, LPCSTR , LPCSTR , float4))&Entity::SetProperty)
            .def("SetProperty", (bool (*)(IGEKEntity *, LPCSTR , LPCSTR , quaternion))&Entity::SetProperty)
            .def("SetProperty", (bool (*)(IGEKEntity *, LPCSTR , LPCSTR , LPCSTR ))&Entity::SetProperty)
        ];

        luabind::module(m_pState)
        [
            luabind::class_<float2>("float2")
            .def_readwrite("x", &float2::x)
            .def_readwrite("y", &float2::y)
            .def(luabind::tostring(luabind::self))
            .def(luabind::constructor<>())
            .def(luabind::constructor<const float2 &>())
            .def(luabind::constructor<const float3 &>())
            .def(luabind::constructor<const float4 &>())
            .def(luabind::constructor<float, float>())
            .def("Dot", &float2::Dot)
            .def("Lerp", &float2::Lerp)
            .def("Distance", &float2::Distance)
            .def("Normalize", &float2::Normalize)
            .def("GetLength", &float2::GetLength)
            .def("GetLengthSqr", &float2::GetLengthSqr)
            .def("GetNormal", &float2::GetNormal)
            .def(luabind::const_self == luabind::other<float2>())
            .def(luabind::const_self < luabind::other<float2>())
            .def(luabind::const_self <= luabind::other<float2>())
            .def(luabind::const_self + luabind::other<float2>())
            .def(luabind::const_self - luabind::other<float2>())
            .def(luabind::const_self + float())
            .def(luabind::const_self - float())
            .def(luabind::const_self * float())
            .def(luabind::const_self / float()),

            luabind::class_<float3>("float3")
            .def_readwrite("x", &float3::x)
            .def_readwrite("y", &float3::y)
            .def_readwrite("z", &float3::z)
            .def(luabind::tostring(luabind::self))
            .def(luabind::constructor<>())
            .def(luabind::constructor<const float2 &>())
            .def(luabind::constructor<const float3 &>())
            .def(luabind::constructor<const float4 &>())
            .def(luabind::constructor<float, float, float>())
            .def("Dot", &float3::Dot)
            .def("Lerp", &float3::Lerp)
            .def("Cross", &float3::Cross)
            .def("Distance", &float3::Distance)
            .def("Normalize", &float3::Normalize)
            .def("GetLength", &float3::GetLength)
            .def("GetLengthSqr", &float3::GetLengthSqr)
            .def("GetNormal", &float3::GetNormal)
            .def(luabind::const_self == luabind::other<float3>())
            .def(luabind::const_self < luabind::other<float3>())
            .def(luabind::const_self <= luabind::other<float3>())
            .def(luabind::const_self + luabind::other<float3>())
            .def(luabind::const_self - luabind::other<float3>())
            .def(luabind::const_self + float())
            .def(luabind::const_self - float())
            .def(luabind::const_self * float())
            .def(luabind::const_self / float()),

            luabind::class_<float4>("float4")
            .def_readwrite("x", &float4::x)
            .def_readwrite("y", &float4::y)
            .def_readwrite("z", &float4::z)
            .def_readwrite("w", &float4::w)
            .def(luabind::tostring(luabind::self))
            .def(luabind::constructor<>())
            .def(luabind::constructor<const float2 &>())
            .def(luabind::constructor<const float3 &>())
            .def(luabind::constructor<const float4 &>())
            .def(luabind::constructor<float, float, float, float>())
            .def("Dot", &float4::Dot)
            .def("Lerp", &float4::Lerp)
            .def("Distance", &float4::Distance)
            .def("Normalize", &float4::Normalize)
            .def("GetLength", &float4::GetLength)
            .def("GetLengthSqr", &float4::GetLengthSqr)
            .def("GetNormal", &float4::GetNormal)
            .def(luabind::const_self == luabind::other<float4>()) 
            .def(luabind::const_self < luabind::other<float4>()) 
            .def(luabind::const_self <= luabind::other<float4>()) 
            .def(luabind::const_self + luabind::other<float4>()) 
            .def(luabind::const_self - luabind::other<float4>()) 
            .def(luabind::const_self + float())
            .def(luabind::const_self - float())
            .def(luabind::const_self * float())
            .def(luabind::const_self / float()),

            luabind::class_<quaternion>("quaternion")
            .def_readwrite("x", &quaternion::x)
            .def_readwrite("y", &quaternion::y)
            .def_readwrite("z", &quaternion::z)
            .def_readwrite("w", &quaternion::w)
            .def(luabind::tostring(luabind::self))
            .def(luabind::constructor<>())
            .def(luabind::constructor<const float4x4 &>()) 
            .def(luabind::constructor<const quaternion &>())
            .def(luabind::constructor<float, float, float, float>())
            .def(luabind::constructor<const float3 &>())
            .def(luabind::constructor<float, float, float>())
            .def(luabind::const_self * luabind::other<quaternion>()) 
            .def(luabind::const_self * luabind::other<float4x4>()),

            luabind::class_<float4x4>("float4x4")
            .def_readwrite("rx", &float4x4::rx)
            .def_readwrite("ry", &float4x4::ry)
            .def_readwrite("rz", &float4x4::rz)
            .def_readwrite("rw", &float4x4::rw)
            .def(luabind::tostring(luabind::self))
            .def(luabind::constructor<>())
            .def(luabind::constructor<const float4x4 &>())
            .def(luabind::constructor<const quaternion &>())
            .def(luabind::constructor<const quaternion &, float3 &>())
            .def(luabind::constructor<const float3 &>())
            .def(luabind::constructor<float, float, float>())
            .def(luabind::const_self * luabind::other<quaternion>())
            .def(luabind::const_self * luabind::other<float4x4>())
        ];
    }

    return CGEKObservable::AddObserver(GetSceneManager(), this);
}

STDMETHODIMP_(void) CGEKComponentSystemScript::Destroy(void)
{
    CGEKObservable::RemoveObserver(GetSceneManager(), this);
    if (m_pState)
    {
        lua_close(m_pState);
    }
}

STDMETHODIMP_(void) CGEKComponentSystemScript::Clear(void)
{
    m_aComponents.clear();
}

STDMETHODIMP CGEKComponentSystemScript::Create(const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent)
{
    HRESULT hRetVal = E_FAIL;
    if (kNode.HasAttribute(L"type") && kNode.GetAttribute(L"type").CompareNoCase(L"script") == 0)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKComponentScript> spComponent(new CGEKComponentScript(pEntity, m_pState));
        if (spComponent)
        {
            CComPtr<IUnknown> spComponentUnknown;
            spComponent->QueryInterface(IID_PPV_ARGS(&spComponentUnknown));
            if (spComponentUnknown)
            {
                GetContext()->RegisterInstance(spComponentUnknown);
            }

            hRetVal = spComponent->QueryInterface(IID_PPV_ARGS(ppComponent));
            if (SUCCEEDED(hRetVal))
            {
                kNode.ListAttributes([&spComponent] (LPCWSTR pName, LPCWSTR pValue) -> void
                {
                    spComponent->SetProperty(pName, pValue);
                } );

                m_aComponents[pEntity] = spComponent;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKComponentSystemScript::Destroy(IGEKEntity *pEntity)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = std::find_if(m_aComponents.begin(), m_aComponents.end(), [&](std::map<IGEKEntity *, CComPtr<CGEKComponentScript>>::value_type &kPair) -> bool
    {
        return (kPair.first == pEntity);
    });

    if (pIterator != m_aComponents.end())
    {
        m_aComponents.erase(pIterator);
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemScript::OnPreUpdate(float nGameTime, float nFrameTime)
{
    for (auto &kPair : m_aComponents)
    {
        if (kPair.second->GetCurrentState().is_valid())
        {
            kPair.second->GetCurrentState()["OnUpdate"](kPair.first, nGameTime, nFrameTime);
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemScript::OnRender(void)
{
    for (auto &kPair : m_aComponents)
    {
        if (kPair.second->GetCurrentState().is_valid() && kPair.second->HasOnRender())
        {
            kPair.second->GetCurrentState()["OnRender"](kPair.first);
        }
    }
}