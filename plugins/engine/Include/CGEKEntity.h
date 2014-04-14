#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <Newton.h>
#include <list>

class CGEKEntity : public CGEKUnknown
                 , public IGEKEntity
{
private:
    CStringW m_strFlags;
    std::map<GEKHASH, CComPtr<IGEKComponent>> m_aComponents;

public:
    CGEKEntity(LPCWSTR pFlags);
    ~CGEKEntity(void);
    DECLARE_UNKNOWN(CGEKEntity)

    HRESULT OnEntityCreated(void);
    HRESULT OnEntityDestroyed(void);
    HRESULT AddComponent(IGEKComponent *pComponent);
    LPCWSTR GetFlags(void);

    // IGEKEntity
    STDMETHOD_(void, ListComponents)            (THIS_ std::function<void(IGEKComponent *)> OnComponent);
    STDMETHOD_(IGEKComponent *, GetComponent)   (THIS_ LPCWSTR pName);
    STDMETHOD(OnEvent)                          (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB);
};