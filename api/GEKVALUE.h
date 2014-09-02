#pragma once

class GEKVALUE
{
public:
    enum TYPE
    {
        UNKNOWN = -1,
        INTEGER = 0,
        FLOAT,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        FLOAT4x4,
        QUATERNION,
        BOOLEAN,
        STRING,
        OBJECT,
    };

private:
    TYPE m_eType;
    union
    {
        LPCVOID m_pPointer;
        LPCWSTR m_pString;
        const float *m_pFloat;
        const UINT32 *m_pInteger;
        const bool *m_pBoolean;
        IUnknown *m_pObject;
    };

public:
    GEKVALUE(void)
        : m_eType(UNKNOWN)
    {
    }

    GEKVALUE(const UINT32 &nValue)
        : m_eType(INTEGER)
        , m_pInteger(&nValue)
    {
    }

    GEKVALUE(const float &nValue)
        : m_eType(FLOAT)
        , m_pFloat(&nValue)
    {
    }

    GEKVALUE(const float2 &nValue)
        : m_eType(FLOAT2)
        , m_pFloat(nValue.xy)
    {
    }

    GEKVALUE(const float3 &nValue)
        : m_eType(FLOAT3)
        , m_pFloat(nValue.xyz)
    {
    }

    GEKVALUE(const float4 &nValue)
        : m_eType(FLOAT4)
        , m_pFloat(nValue.xyzw)
    {
    }

    GEKVALUE(const float4x4 &nValue)
        : m_eType(FLOAT4x4)
        , m_pFloat(nValue.data)
    {
    }

    GEKVALUE(const quaternion &nValue)
        : m_eType(QUATERNION)
        , m_pFloat(nValue.xyzw)
    {
    }

    GEKVALUE(const bool &bValue)
        : m_eType(BOOLEAN)
        , m_pBoolean(&bValue)
    {
    }

    GEKVALUE(LPCWSTR pValue)
        : m_eType(STRING)
        , m_pString(pValue)
    {
    }

    GEKVALUE(IUnknown *pUnknown)
        : m_eType(OBJECT)
        , m_pObject(pUnknown)
    {
    }

    TYPE GetType(void) const
    {
        return m_eType;
    }

    UINT32 GetUINT32(void) const
    {
        switch (m_eType)
        {
        case INTEGER:       return (*m_pInteger);
        case FLOAT:         return UINT32(*m_pFloat);
        case BOOLEAN:       return ((*m_pBoolean) ? 1 : 0);
        case STRING:        return StrToUINT32(m_pString);
        };

        return 0;
    }

    float GetFloat(void) const
    {
        switch (m_eType)
        {
        case INTEGER:       return float(*m_pInteger);
        case FLOAT:         return (*m_pFloat);
        case BOOLEAN:       return ((*m_pBoolean) ? 1.0f : 0.0f);
        case STRING:        return StrToFloat(m_pString);
        };

        return 0.0f;
    }

    float2 GetFloat2(void) const
    {
        switch (m_eType)
        {
        case FLOAT2:        return *(float2 *)m_pFloat;
        case FLOAT3:        return *(float2 *)m_pFloat;
        case FLOAT4:        return *(float2 *)m_pFloat;
        case STRING:        return StrToFloat2(m_pString);
        };

        return float2();
    }

    float3 GetFloat3(void) const
    {
        switch (m_eType)
        {
        case FLOAT3:        return *(float3 *)m_pFloat;
        case FLOAT4:        return *(float3 *)m_pFloat;
        case STRING:        return StrToFloat3(m_pString);
        };

        return float3();
    }

    float4 GetFloat4(void) const
    {
        switch (m_eType)
        {
        case FLOAT4:        return *(float4 *)m_pFloat;
        case QUATERNION:    return *(float4 *)m_pFloat;
        case STRING:        return StrToFloat4(m_pString);
        };

        return float4();
    }

    float4x4 GetFloat4x4(void) const
    {
        switch (m_eType)
        {
        case FLOAT4x4:      return *(float4x4 *)m_pFloat;
        case QUATERNION:    return *(quaternion *)m_pFloat;
        case STRING:        return StrToQuaternion(m_pString);
        };

        return quaternion();
    }

    quaternion GetQuaternion(void) const
    {
        switch (m_eType)
        {
        case FLOAT4:        return *(quaternion *)m_pFloat;
        case FLOAT4x4:      return *(float4x4 *)m_pFloat;
        case QUATERNION:    return *(quaternion *)m_pFloat;
        case STRING:        return StrToQuaternion(m_pString);
        };

        return quaternion();
    }

    bool GetBoolean(void) const
    {
        switch (m_eType)
        {
        case INTEGER:       return ((*m_pInteger) ? true : false);
        case BOOLEAN:       return (*m_pBoolean);
        case STRING:        return StrToBoolean(m_pString);
        };

        return false;
    }

    LPCWSTR GetRawString(void) const
    {
        switch (m_eType)
        {
        case STRING:        return m_pString;
        };

        return nullptr;
    }

    CStringW GetString(void) const
    {
        switch (m_eType)
        {
        case INTEGER:       return FormatString(L"%d", (*m_pInteger));
        case FLOAT:         return FormatString(L"%f", (*m_pFloat));
        case FLOAT2:        return FormatString(L"%f, %f", m_pFloat[0], m_pFloat[1]);
        case FLOAT3:        return FormatString(L"%f, %f, %f", m_pFloat[0], m_pFloat[1], m_pFloat[2]);
        case FLOAT4:        return FormatString(L"%f, %f, %f, %f", m_pFloat[0], m_pFloat[1], m_pFloat[2], m_pFloat[3]);
        case QUATERNION:    return FormatString(L"%f, %f, %f, %f", m_pFloat[0], m_pFloat[1], m_pFloat[2], m_pFloat[3]);
        case BOOLEAN:       return ((*m_pBoolean) ? L"true" : L"false");
        case STRING:        return m_pString;
        case OBJECT:        return FormatString(L"%d", m_pObject);
        };

        return L"";
    }

    IUnknown *GetObject(void) const
    {
        switch (m_eType)
        {
        case OBJECT:        return m_pObject;
        };

        return nullptr;
    }

    GEKVALUE &operator = (const GEKVALUE &kProperty)
    {
        m_eType = kProperty.m_eType;
        m_pPointer = kProperty.m_pPointer;
        return (*this);
    }
};
