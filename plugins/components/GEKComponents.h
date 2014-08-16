#pragma once

#include <vector>
#include <hash_map>

template <struct DATA>
class TGEKComponentPool
{
private:
    std::hash_map<GEKENTITYID, UINT32> m_aEntityMap;
    std::vector<DATA> m_aDataPool;

public:
    HRESULT AddComponent(const GEKENTITYID &nEntityID)
    {
        m_
    }

    HRESULT RemoveComponent(const GEKENTITYID &nEntityID);
    BOOL HasComponent(const GEKENTITYID &nEntityID);
    DATA &GetData(const GEKENTITYID &nEntityID);
    const DATA &GetData(const GEKENTITYID &nEntityID) const;
};