#include "CGEKComponentNewton.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"
#include <dNewtonCollision.h>

#ifdef _DEBUG
    #pragma comment(lib, "newton.lib")
    #pragma comment(lib, "dNewton.lib")
    #pragma comment(lib, "dContainers.lib")
#else
    #pragma comment(lib, "newton.lib")
    #pragma comment(lib, "dNewton.lib")
    #pragma comment(lib, "dContainers.lib")
#endif

REGISTER_COMPONENT(dynamicbody)
    REGISTER_COMPONENT_DEFAULT_VALUE(shape, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(params, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(material, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(mass, 0.0f)
    REGISTER_COMPONENT_SERIALIZE(dynamicbody)
        REGISTER_COMPONENT_SERIALIZE_VALUE(shape, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(mass, StrFromFloat)
    REGISTER_COMPONENT_DESERIALIZE(dynamicbody)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(shape, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(mass, StrToFloat)
END_REGISTER_COMPONENT(dynamicbody)

REGISTER_COMPONENT(player)
    REGISTER_COMPONENT_DEFAULT_VALUE(mass, 100.0f)
    REGISTER_COMPONENT_DEFAULT_VALUE(outer_radius, 1.0f)
    REGISTER_COMPONENT_DEFAULT_VALUE(inner_radius, 0.25f)
    REGISTER_COMPONENT_DEFAULT_VALUE(height, 1.9f)
    REGISTER_COMPONENT_DEFAULT_VALUE(stair_step, 0.25f)
    REGISTER_COMPONENT_SERIALIZE(player)
        REGISTER_COMPONENT_SERIALIZE_VALUE(mass, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(outer_radius, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(inner_radius, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(height, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(stair_step, StrFromFloat)
    REGISTER_COMPONENT_DESERIALIZE(player)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(mass, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(outer_radius, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(inner_radius, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(height, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(stair_step, StrToFloat)
END_REGISTER_COMPONENT(player)

class CGEKNewtonBody
{
private:
    IGEKEngineCore *m_pEngine;
    IGEKNewtonSystem *m_pNewton;
    GEKENTITYID m_nEntityID;

public:
    CGEKNewtonBody(IGEKEngineCore *pEngine, IGEKNewtonSystem *pNewton, const GEKENTITYID &nEntityID)
        : m_pEngine(pEngine)
        , m_pNewton(pNewton)
        , m_nEntityID(nEntityID)
    {
    }

    IGEKEngineCore *GetEngineCore(void)
    {
        return m_pEngine;
    }

    IGEKNewtonSystem *GetNewtonSystem(void)
    {
        return m_pNewton;
    }

    GEKENTITYID GetEntityID(void) const
    {
        return m_nEntityID;
    }
};

class CGEKDynamicBody : public CGEKUnknown
                      , public CGEKNewtonBody
                      , public dNewtonDynamicBody
{
public:
    DECLARE_UNKNOWN(CGEKDynamicBody)
    CGEKDynamicBody(IGEKEngineCore *pEngine, IGEKNewtonSystem *pNewton, const GEKENTITYID &nEntityID, float nMass, const dNewtonCollision* const pCollision, const float4x4& nMatrix)
        : CGEKNewtonBody(pEngine, pNewton, nEntityID)
        , dNewtonDynamicBody(pNewton->GetCore(), nMass, pCollision, nullptr, nMatrix.data, NULL)
    {
    }

    // dNewtonDynamicBody
    void OnBodyTransform(const dFloat* const pMatrix, int nThreadID)
    {
        const float4x4 &nMatrix = *reinterpret_cast<const float4x4 *>(pMatrix);
        auto &kTransform = GetEngineCore()->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(GetEntityID(), GET_COMPONENT_ID(transform));
        kTransform.position = nMatrix.t;
        kTransform.rotation = nMatrix;
    }

    void OnForceAndTorque(dFloat nTimeStep, int nThreadID)
    {
        auto &kDynamicBody = GetEngineCore()->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(dynamicbody)>(GetEntityID(), GET_COMPONENT_ID(dynamicbody));
        AddForce((GetNewtonSystem()->GetGravity() * kDynamicBody.mass).xyz);
    }
};

BEGIN_INTERFACE_LIST(CGEKDynamicBody)
END_INTERFACE_LIST_UNKNOWN

class CGEKPlayer : public CGEKUnknown 
                 , public CGEKNewtonBody
                 , public dNewtonPlayerManager::dNewtonPlayer
                 , public IGEKInputObserver
{
private:
    float m_nTurn;
    concurrency::concurrent_unordered_map<CStringW, float> m_aConstantActions;
    concurrency::concurrent_unordered_map<CStringW, float> m_aSingleActions;

public:
    DECLARE_UNKNOWN(CGEKPlayer)
    CGEKPlayer(IGEKEngineCore *pEngine, IGEKNewtonSystem *pNewton, const GEKENTITYID &nEntityID, float nMass, float nOuterRadius, float nInnerRadius, float nHeight, float nStairStep)
        : CGEKNewtonBody(pEngine, pNewton, nEntityID)
        , dNewtonPlayer(pNewton->GetPlayerManager(), nullptr, nMass, nOuterRadius, nInnerRadius, nHeight, nStairStep, float3(0.0f, 1.0f, 0.0f).xyz, float3(0.0f, 0.0f, 1.0f).xyz, 1)
        , m_nTurn(0.0f)
    {
    }

    ~CGEKPlayer(void)
    {
        CGEKObservable::RemoveObserver(GetEngineCore(), GetClass<IGEKInputObserver>());
    }

    // dNewtonPlayerManager::dNewtonPlayer
    void OnBodyTransform(const dFloat* const pMatrix, int nThreadID)
    {
        const float4x4 &nMatrix = *reinterpret_cast<const float4x4 *>(pMatrix);
        auto &kTransform = GetEngineCore()->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(GetEntityID(), GET_COMPONENT_ID(transform));
        kTransform.position = nMatrix.t;
        kTransform.rotation = nMatrix;
    }

    void OnPlayerMove(dFloat nTimeStep)
    {
        float nForward = 0.0f;
        float nStrafe = 0.0f;
        float nHeight = 0.0f;
        auto GetInput = [&](concurrency::concurrent_unordered_map<CStringW, float> &aActions) -> void
        {
            m_nTurn += (aActions[L"turn"] * 0.01f);

            nForward += aActions[L"forward"];
            nForward -= aActions[L"backward"];
            nStrafe += aActions[L"strafe_left"];
            nStrafe -= aActions[L"strafe_right"];
            nHeight += aActions[L"rise"];
            nHeight -= aActions[L"fall"];
        };

        GetInput(m_aSingleActions);
        m_aSingleActions.clear();
        GetInput(m_aConstantActions);

        auto &kPlayer = GetEngineCore()->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(player)>(GetEntityID(), GET_COMPONENT_ID(player));
        SetPlayerVelocity(nForward, nStrafe, nHeight, m_nTurn, GetNewtonSystem()->GetGravity().xyz, nTimeStep);
    }

    // IGEKInputObserver
    STDMETHODIMP_(void) OnState(LPCWSTR pName, bool bState)
    {
        if (bState)
        {
            m_aConstantActions[pName] = 1.0f;
        }
        else
        {
            m_aConstantActions[pName] = 0.0f;
        }
    }

    STDMETHODIMP_(void) OnValue(LPCWSTR pName, float nValue)
    {
        m_aSingleActions[pName] = nValue;
    }
};

BEGIN_INTERFACE_LIST(CGEKPlayer)
    INTERFACE_LIST_ENTRY_COM(IGEKInputObserver)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKComponentSystemNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKNewtonSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemNewton)

CGEKComponentSystemNewton::CGEKComponentSystemNewton(void)
    : m_pEngine(nullptr)
    , m_nGravity(0.0f, -9.8331f, 0.0f)
{
}

CGEKComponentSystemNewton::~CGEKComponentSystemNewton(void)
{
    CGEKObservable::RemoveObserver(m_pEngine->GetSceneManager(), GetClass<IGEKSceneObserver>());

    m_aBodies.clear();
    m_aCollisions.clear();
    m_aMaterials.clear();
    m_spPlayerManager.reset();
    DestroyAllBodies();
}

CGEKComponentSystemNewton::MATERIAL *CGEKComponentSystemNewton::LoadMaterial(LPCWSTR pName)
{
    REQUIRE_RETURN(pName, nullptr);

    MATERIAL *pMaterial = nullptr;
    auto pIterator = m_aMaterials.find(pName);
    if (pIterator != m_aMaterials.end())
    {
        pMaterial = &(*pIterator).second;
    }
    else
    {
        CLibXMLDoc kDocument;
        if (SUCCEEDED(kDocument.Load(FormatString(L"%%root%%\\data\\materials\\%s.xml", pName))))
        {
            CLibXMLNode kMaterialNode = kDocument.GetRoot();
            if (kMaterialNode)
            {
                CLibXMLNode kSurfaceNode = kMaterialNode.FirstChildElement(L"surface");
                if (kSurfaceNode)
                {
                    MATERIAL &kMaterial = m_aMaterials[pName];
                    if (kSurfaceNode.HasAttribute(L"staticfriction"))
                    {
                        kMaterial.m_nStaticFriction = StrToFloat(kSurfaceNode.GetAttribute(L"staticfriction"));
                    }

                    if (kSurfaceNode.HasAttribute(L"kineticfriction"))
                    {
                        kMaterial.m_nKineticFriction = StrToFloat(kSurfaceNode.GetAttribute(L"kineticfriction"));
                    }

                    if (kSurfaceNode.HasAttribute(L"elasticity"))
                    {
                        kMaterial.m_nElasticity = StrToFloat(kSurfaceNode.GetAttribute(L"elasticity"));
                    }

                    if (kSurfaceNode.HasAttribute(L"softness"))
                    {
                        kMaterial.m_nSoftness = StrToFloat(kSurfaceNode.GetAttribute(L"softness"));
                    }

                    pMaterial = &kMaterial;
                }
            }
        }
    }

    if (pMaterial)
    {
        return pMaterial;
    }
    else
    {
        return &m_kDefaultMaterial;
    }
}

dNewtonCollision *CGEKComponentSystemNewton::LoadCollision(LPCWSTR pShape, LPCWSTR pParams)
{
    dNewtonCollision *pCollision = nullptr;

    CStringW strIdentity;
    strIdentity.Format(L"%s|%s", pShape, pParams);
    auto pIterator = m_aCollisions.find(strIdentity);
    if (pIterator != m_aCollisions.end())
    {
        pCollision = (*pIterator).second.get();
    }
    else
    {
        size_t nHash = std::hash<LPCWSTR>()(strIdentity.GetString());
        if (_wcsicmp(pShape, L"convex_hull") == 0 ||
            _wcsicmp(pShape, L"tree") == 0)
        {
            std::vector<UINT8> aBuffer;
            HRESULT hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.gek", pParams), aBuffer);
            if (SUCCEEDED(hRetVal))
            {
                UINT8 *pBuffer = &aBuffer[0];
                UINT32 nGEKX = *((UINT32 *)pBuffer);
                pBuffer += sizeof(UINT32);

                UINT16 nType = *((UINT16 *)pBuffer);
                pBuffer += sizeof(UINT16);

                UINT16 nVersion = *((UINT16 *)pBuffer);
                pBuffer += sizeof(UINT16);

                if (nGEKX == *(UINT32 *)"GEKX" && nType == 0 && nVersion == 2)
                {
                    aabb nAABB = *(aabb *)pBuffer;
                    pBuffer += sizeof(aabb);

                    UINT32 nNumMaterials = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    struct RENDERMATERIAL
                    {
                        UINT32 m_nFirstVertex;
                        UINT32 m_nFirstIndex;
                        UINT32 m_nNumIndices;
                    };

                    std::map<CStringW, RENDERMATERIAL> aMaterials;
                    for (UINT32 nMaterial = 0; nMaterial < nNumMaterials; ++nMaterial)
                    {
                        CStringA strMaterialUTF8 = pBuffer;
                        pBuffer += (strMaterialUTF8.GetLength() + 1);
                        CStringW strMaterial(CA2W(strMaterialUTF8, CP_UTF8));

                        RENDERMATERIAL &kMaterial = aMaterials[strMaterial];
                        kMaterial.m_nFirstVertex = *((UINT32 *)pBuffer);
                        pBuffer += sizeof(UINT32);

                        kMaterial.m_nFirstIndex = *((UINT32 *)pBuffer);
                        pBuffer += sizeof(UINT32);

                        kMaterial.m_nNumIndices = *((UINT32 *)pBuffer);
                        pBuffer += sizeof(UINT32);
                    }

                    UINT32 nNumVertices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    float3 *pVertices = (float3 *)pBuffer;
                    pBuffer += (sizeof(float3) * nNumVertices);
                    pBuffer += (sizeof(float2) * nNumVertices);
                    pBuffer += (sizeof(float3) * nNumVertices);

                    UINT32 nNumIndices = *((UINT32 *)pBuffer);
                    pBuffer += sizeof(UINT32);

                    UINT16 *pIndices = (UINT16 *)pBuffer;

                    if (_wcsicmp(pShape, L"convex_hull") == 0 && aMaterials.size() > 0)
                    {
                        std::vector<float3> aCloud(nNumIndices);
                        for (UINT32 nIndex = 0; nIndex < nNumIndices; ++nIndex)
                        {
                            aCloud[nIndex] = pVertices[pIndices[nIndex]];
                        }

                        pCollision = new dNewtonCollisionConvexHull(this, aCloud.size(), aCloud[0].xyz, sizeof(float3), 0.025f, nHash);
                    }
                    else if (_wcsicmp(pShape, L"tree") == 0)
                    {
#ifdef _DEBUG
                        dNewtonCollisionMesh *pMesh = new dNewtonCollisionMesh(this, 0);
#else
                        dNewtonCollisionMesh *pMesh = new dNewtonCollisionMesh(this, 1);
#endif
                        if (pMesh != nullptr)
                        {
                            pMesh->BeginFace();
                            for (auto &kPair : aMaterials)
                            {
                                RENDERMATERIAL &kMaterial = kPair.second;
                                MATERIAL *pMaterial = LoadMaterial(kPair.first);
                                for (UINT32 nIndex = 0; nIndex < kMaterial.m_nNumIndices; nIndex += 3)
                                {
                                    float3 aFace[3] =
                                    {
                                        pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 0]],
                                        pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 1]],
                                        pVertices[kMaterial.m_nFirstVertex + pIndices[kMaterial.m_nFirstIndex + nIndex + 2]],
                                    };

                                    pMesh->AddFace(3, aFace[0].xyz, sizeof(float3), int(pMaterial));
                                }
                            }

                            pMesh->EndFace();
                            pCollision = pMesh;
                        }
                    }
                }
            }
        }
        else
        {
            if (_wcsicmp(pShape, L"cube") == 0)
            {
                float3 nSize = StrToFloat3(pParams);
                pCollision = new dNewtonCollisionBox(this, nSize.x, nSize.y, nSize.z, nHash);
            }
            else if (_wcsicmp(pShape, L"sphere") == 0)
            {
                float nSize = StrToFloat(pParams);
                pCollision = new dNewtonCollisionSphere(this, nSize, nHash);
            }
            else if (_wcsicmp(pShape, L"cone") == 0)
            {
                float2 nSize = StrToFloat2(pParams);
                pCollision = new dNewtonCollisionCone(this, nSize.x, nSize.y, nHash);
            }
            else if (_wcsicmp(pShape, L"capsule") == 0)
            {
                float2 nSize = StrToFloat2(pParams);
                pCollision = new dNewtonCollisionCapsule(this, nSize.x, nSize.y, nHash);
            }
            else if (_wcsicmp(pShape, L"cylinder") == 0)
            {
                float2 nSize = StrToFloat2(pParams);
                pCollision = new dNewtonCollisionCylinder(this, nSize.x, nSize.y, nHash);
            }
            else if (_wcsicmp(pShape, L"tapered_capsule") == 0)
            {
                float3 nSize = StrToFloat3(pParams);
                pCollision = new dNewtonCollisionTaperedCapsule(this, nSize.x, nSize.y, nSize.z, nHash);
            }
            else if (_wcsicmp(pShape, L"tapered_cylinder") == 0)
            {
                float3 nSize = StrToFloat3(pParams);
                pCollision = new dNewtonCollisionTaperedCylinder(this, nSize.x, nSize.y, nSize.z, nHash);
            }
            else if (_wcsicmp(pShape, L"chamfer_cylinder") == 0)
            {
                float2 nSize = StrToFloat2(pParams);
                pCollision = new dNewtonCollisionChamferedCylinder(this, nSize.x, nSize.y, nHash);
            }
        }

        if (pCollision)
        {
            m_aCollisions[strIdentity].reset(pCollision);
        }
    }

    return pCollision;
}

bool CGEKComponentSystemNewton::OnBodiesAABBOverlap(const dNewtonBody* const pBody0, const dNewtonBody* const pBody1, int nThreadID) const
{
    return true;
}

bool CGEKComponentSystemNewton::OnCompoundSubCollisionAABBOverlap(const dNewtonBody* const pBody0, const dNewtonCollision* const pSubShape0, const dNewtonBody* const pBody1, const dNewtonCollision* const pSubShape1, int nThreadID) const
{
    return true;
}

void CGEKComponentSystemNewton::OnContactProcess(dNewtonContactMaterial* const pContactMaterial, dFloat nTimeStep, int nThreadID)
{
    CGEKDynamicBody *pBody0 = dynamic_cast<CGEKDynamicBody *>(pContactMaterial->GetBody0());
    CGEKDynamicBody *pBody1 = dynamic_cast<CGEKDynamicBody *>(pContactMaterial->GetBody1());
    if (pBody0 && pBody1)
    {
        NewtonWorldCriticalSectionLock(GetNewton(), nThreadID);
        for (void *pContact = pContactMaterial->GetFirstContact(); pContact; pContact = pContactMaterial->GetNextContact(pContact))
        {
            NewtonMaterial *pMaterial = NewtonContactGetMaterial(pContact);
            auto &kNewton0 = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(dynamicbody)>(pBody0->GetEntityID(), GET_COMPONENT_ID(dynamicbody));
            auto &kNewton1 = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(dynamicbody)>(pBody1->GetEntityID(), GET_COMPONENT_ID(dynamicbody));
            MATERIAL *pMaterial0 = LoadMaterial(kNewton0.material);
            MATERIAL *pMaterial1 = LoadMaterial(kNewton1.material);

            UINT32 nFaceAttribute = NewtonMaterialGetContactFaceAttribute(pMaterial);
            if (nFaceAttribute > 0)
            {
                if (kNewton0.material.CompareNoCase(L"tree") == 0)
                {
                    pMaterial0 = (MATERIAL *)nFaceAttribute;
                }
                else if (kNewton1.material.CompareNoCase(L"tree") == 0)
                {
                    pMaterial1 = (MATERIAL *)nFaceAttribute;
                }
            }

            NewtonMaterialSetContactSoftness(pMaterial, ((pMaterial0->m_nSoftness + pMaterial1->m_nSoftness) * 0.5f));
            NewtonMaterialSetContactElasticity(pMaterial, ((pMaterial0->m_nElasticity + pMaterial1->m_nElasticity) * 0.5f));
            NewtonMaterialSetContactFrictionCoef(pMaterial, pMaterial0->m_nStaticFriction, pMaterial0->m_nKineticFriction, 0);
            NewtonMaterialSetContactFrictionCoef(pMaterial, pMaterial1->m_nStaticFriction, pMaterial1->m_nKineticFriction, 1);

            float3 nPosition, nNormal;
            NewtonMaterialGetContactPositionAndNormal(pMaterial, pBody0->GetNewtonBody(), nPosition.xyz, nNormal.xyz);
            CGEKObservable::SendEvent(TGEKEvent<IGEKNewtonObserver>(std::bind(&IGEKNewtonObserver::OnCollision, std::placeholders::_1, pBody0->GetEntityID(), pBody1->GetEntityID(), nPosition, nNormal)));
        }
    }


    NewtonWorldCriticalSectionUnlock(GetNewton());
}
STDMETHODIMP CGEKComponentSystemNewton::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine->GetSceneManager(), GetClass<IGEKSceneObserver>());
    if (SUCCEEDED(hRetVal))
    {
        m_spPlayerManager.reset(new dNewtonPlayerManager(this));
        hRetVal = (m_spPlayerManager ? S_OK : E_OUTOFMEMORY);
    }

    return hRetVal;
};

STDMETHODIMP CGEKComponentSystemNewton::OnLoadEnd(HRESULT hRetVal)
{
    if (FAILED(hRetVal))
    {
        OnFree();
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnFree(void)
{
    m_aCollisions.clear();
    m_aBodies.clear();
    m_aMaterials.clear();
    DestroyAllBodies();
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnEntityDestroyed(const GEKENTITYID &nEntityID)
{
    auto pIterator = m_aBodies.find(nEntityID);
    if (pIterator != m_aBodies.end())
    {
        m_aBodies.unsafe_erase(pIterator);
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnComponentAdded(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    if (nComponentID == GET_COMPONENT_ID(dynamicbody))
    {
        if (m_pEngine->GetSceneManager()->HasComponent(nEntityID, GET_COMPONENT_ID(transform)))
        {
            auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
            auto &kDynamicBody = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(dynamicbody)>(nEntityID, GET_COMPONENT_ID(dynamicbody));
            if (!kDynamicBody.shape.IsEmpty())
            {
                dNewtonCollision *pCollision = LoadCollision(kDynamicBody.shape, kDynamicBody.params);
                if (pCollision != nullptr)
                {
                    float4x4 nMatrix(kTransform.rotation, kTransform.position);
                    CComPtr<IGEKUnknown> spBody = new CGEKDynamicBody(m_pEngine, this, nEntityID, kDynamicBody.mass, pCollision, nMatrix);
                    if (spBody)
                    {
                        m_aBodies[nEntityID] = spBody;
                    }
                }
            }
        }
    }
    else if (nComponentID == GET_COMPONENT_ID(player))
    {
        if (m_pEngine->GetSceneManager()->HasComponent(nEntityID, GET_COMPONENT_ID(transform)))
        {
            auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
            auto &kPlayer = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(player)>(nEntityID, GET_COMPONENT_ID(player));
            CComPtr<CGEKPlayer> spPlayer = new CGEKPlayer(m_pEngine, this, nEntityID, kPlayer.mass, kPlayer.outer_radius, kPlayer.inner_radius, kPlayer.height, kPlayer.stair_step);
            if (spPlayer)
            {
                CGEKObservable::AddObserver(m_pEngine, spPlayer->GetClass<IGEKInputObserver>());
                spPlayer->SetMatrix(float4x4(kTransform.rotation, kTransform.position).data);
                if (spPlayer)
                {
                    spPlayer.QueryInterface(&m_aBodies[nEntityID]);
                }
            }
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnComponentRemoved(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    if (nComponentID == GET_COMPONENT_ID(dynamicbody) ||
        nComponentID == GET_COMPONENT_ID(player))
    {
        auto pIterator = m_aBodies.find(nEntityID);
        if (pIterator != m_aBodies.end())
        {
            m_aBodies.unsafe_erase(pIterator);
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnUpdate(float nGameTime, float nFrameTime)
{
    Update(nFrameTime);
}

STDMETHODIMP_(dNewton *) CGEKComponentSystemNewton::GetCore(void)
{
    return this;
}

STDMETHODIMP_(dNewtonPlayerManager *) CGEKComponentSystemNewton::GetPlayerManager(void)
{
    return m_spPlayerManager.get();
}

STDMETHODIMP_(float3) CGEKComponentSystemNewton::GetGravity(void)
{
    return m_nGravity;
}
