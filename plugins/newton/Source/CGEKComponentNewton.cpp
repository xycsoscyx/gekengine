#include "CGEKComponentNewton.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"
#include <dNewtonCollision.h>

#pragma comment(lib, "newton.lib")
#pragma comment(lib, "dNewton.lib")

REGISTER_COMPONENT(newton)
    REGISTER_COMPONENT_DEFAULT_VALUE(shape, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(params, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(material, L"")
    REGISTER_COMPONENT_DEFAULT_VALUE(mass, 0.0f)
    REGISTER_COMPONENT_SERIALIZE(newton)
        REGISTER_COMPONENT_SERIALIZE_VALUE(shape, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_SERIALIZE_VALUE(mass, StrFromFloat)
    REGISTER_COMPONENT_DESERIALIZE(newton)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(shape, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(params, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(material, )
        REGISTER_COMPONENT_DESERIALIZE_VALUE(mass, StrToFloat)
END_REGISTER_COMPONENT(newton)

BEGIN_INTERFACE_LIST(CGEKComponentSystemNewton)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKComponentSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKNewton)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKComponentSystemNewton)

class CGEKDynamicBody : public dNewtonDynamicBody
{
private:
    IGEKEngineCore *m_pEngine;
    IGEKNewton *m_pNewton;

    GEKENTITYID m_nEntityID;

public:
    CGEKDynamicBody(IGEKEngineCore *pEngine, IGEKNewton *pNewton, float nMass, const dNewtonCollision* const pCollision, const float4x4& nMatrix, const GEKENTITYID &nEntityID)
        : dNewtonDynamicBody(pNewton->GetCore(), nMass, pCollision, nullptr, nMatrix.data, NULL)
        , m_pEngine(pEngine)
        , m_pNewton(pNewton)
        , m_nEntityID(nEntityID)
    {
    }

    GEKENTITYID GetEntityID(void) const
    {
        return m_nEntityID;
    }

    void OnBodyTransform(const dFloat* const pMatrix, int nThreadID)
    {
        const float4x4 &nMatrix = *reinterpret_cast<const float4x4 *>(pMatrix);
        auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(m_nEntityID, GET_COMPONENT_ID(transform));
        kTransform.position = nMatrix.t;
        kTransform.rotation = nMatrix;
    }

    void OnForceAndTorque(dFloat nTimeStep, int nThreadID)
    {
        auto &kNewton = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(newton)>(m_nEntityID, GET_COMPONENT_ID(newton));
        AddForce((m_pNewton->GetGravity() * kNewton.mass).xyz);
    }
};

CGEKComponentSystemNewton::CGEKComponentSystemNewton(void)
    : m_pEngine(nullptr)
    , m_nGravity(0.0f, -9.8331f, 0.0f)
{
    NewtonWorld *pNewton = NewtonCreate();
    NewtonDestroy(pNewton);
}

CGEKComponentSystemNewton::~CGEKComponentSystemNewton(void)
{
    CGEKObservable::RemoveObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());

    m_aBodies.clear();
    m_aCollisions.clear();
    m_aMaterials.clear();
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
    CStringW strIdentity;
    strIdentity.Format(L"%s|%s", pShape, pParams);
    auto pIterator = m_aCollisions.find(strIdentity);
    if (pIterator != m_aCollisions.end())
    {
        return (*pIterator).second.get();
    }

    size_t nHash = std::hash<LPCWSTR>()(strIdentity.GetString());

    dNewtonCollision *pCollision = nullptr;
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
                    dNewtonCollisionMesh *pMesh = new dNewtonCollisionMesh(this, 1);
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

    NewtonWorldCriticalSectionLock(GetNewton(), nThreadID);
    for (void *pContact = pContactMaterial->GetFirstContact(); pContact; pContact = pContactMaterial->GetNextContact(pContact))
    {
        NewtonMaterial *pMaterial = NewtonContactGetMaterial(pContact);
        auto &kNewton0 = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(newton)>(pBody0->GetEntityID(), GET_COMPONENT_ID(newton));
        auto &kNewton1 = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(newton)>(pBody1->GetEntityID(), GET_COMPONENT_ID(newton));
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

    NewtonWorldCriticalSectionUnlock(GetNewton());
}
STDMETHODIMP CGEKComponentSystemNewton::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = CGEKObservable::AddObserver(m_pEngine->GetSceneManager(), (IGEKSceneObserver *)GetUnknown());

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
    m_aBodies.clear();
    m_aCollisions.clear();
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
    if (nComponentID == GET_COMPONENT_ID(newton))
    {
        if (m_pEngine->GetSceneManager()->HasComponent(nEntityID, GET_COMPONENT_ID(transform)))
        {
            auto &kTransform = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(transform)>(nEntityID, GET_COMPONENT_ID(transform));
            auto &kNewton = m_pEngine->GetSceneManager()->GetComponent<GET_COMPONENT_DATA(newton)>(nEntityID, GET_COMPONENT_ID(newton));
            if (!kNewton.shape.IsEmpty())
            {
                float4x4 nMatrix;
                nMatrix = kTransform.rotation;
                nMatrix.t = kTransform.position;
                dNewtonCollision *pCollision = LoadCollision(kNewton.shape, kNewton.params);
                if (pCollision != nullptr)
                {
                    CGEKDynamicBody *pBody = new CGEKDynamicBody(m_pEngine, this, kNewton.mass, pCollision, nMatrix, nEntityID);
                    if (pBody != nullptr)
                    {
                        m_aBodies[nEntityID].reset(pBody);
                    }
                }
            }
        }
    }
}

STDMETHODIMP_(void) CGEKComponentSystemNewton::OnComponentRemoved(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    if (nComponentID == GET_COMPONENT_ID(newton))
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

STDMETHODIMP_(float3) CGEKComponentSystemNewton::GetGravity(void)
{
    return m_nGravity;
}
