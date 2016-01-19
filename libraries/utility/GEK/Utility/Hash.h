#pragma once

#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <algorithm>
#include <string>

namespace std
{
    template<typename CHARTYPE, typename TRAITSTYPE>
    struct hash<ATL::CStringT<CHARTYPE, TRAITSTYPE>> : public unary_function<ATL::CStringT<CHARTYPE, TRAITSTYPE>, size_t>
    {
        size_t operator()(const ATL::CStringT<CHARTYPE, TRAITSTYPE> &string) const
        {
            return CStringElementTraits<typename TRAITSTYPE>::Hash(string);
        }
    };

    template<typename CHARTYPE, typename TRAITSTYPE>
    struct equal_to<ATL::CStringT<CHARTYPE, TRAITSTYPE>> : public unary_function<ATL::CStringT<CHARTYPE, TRAITSTYPE>, bool>
    {
        bool operator()(const ATL::CStringT<CHARTYPE, TRAITSTYPE> &leftString, const ATL::CStringT<CHARTYPE, TRAITSTYPE> &rightString) const
        {
            return (leftString == rightString);
        }
    };

    template <>
    struct hash<GUID> : public unary_function<GUID, size_t>
    {
        size_t operator()(REFGUID guid) const
        {
            DWORD *last = (DWORD *)&guid.Data4[0];
            return (guid.Data1 ^ (guid.Data2 << 16 | guid.Data3) ^ (last[0] | last[1]));
        }
    };

    template <>
    struct equal_to<GUID> : public unary_function<GUID, bool>
    {
        bool operator()(REFGUID leftGuid, REFGUID rightGuid) const
        {
            return (memcmp(&leftGuid, &rightGuid, sizeof(GUID)) == 0);
        }
    };

    template <>
    struct hash<LPCSTR>
    {
        size_t operator()(const LPCSTR &value) const
        {
            return hash<string>()(string(value));
        }
    };

    template <>
    struct hash<LPCWSTR>
    {
        size_t operator()(const LPCWSTR &value) const
        {
            return hash<wstring>()(wstring(value));
        }
    };

    inline size_t hash_combine(const size_t upper, const size_t lower)
    {
        return upper ^ (lower + 0x9e3779b9 + (upper << 6) + (upper >> 2));
    }

    inline size_t hash_combine(void)
    {
        return 0;
    }

    template <typename T, typename... Ts>
    size_t hash_combine(const T& t, const Ts&... ts)
    {
        size_t seed = hash<T>()(t);
        if (sizeof...(ts) == 0)
        {
            return seed;
        }

        size_t remainder = hash_combine(ts...);
        return hash_combine(seed, remainder);
    }

    template <typename CLASS>
    struct hash<CComPtr<CLASS>>
    {
        size_t operator()(const CComPtr<CLASS> &value) const
        {
            return hash<LPCVOID>()(value.p);
        }
    };

    template <typename CLASS>
    struct hash<CComQIPtr<CLASS>>
    {
        size_t operator()(const CComQIPtr<CLASS> &value) const
        {
            return hash<LPCVOID>()(value.p);
        }
    };
};

__forceinline
bool operator < (REFGUID leftGuid, REFGUID rightGuid)
{
    return (memcmp(&leftGuid, &rightGuid, sizeof(GUID)) < 0);
}
