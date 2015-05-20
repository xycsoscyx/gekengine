#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Utility\Common.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Model
            {
                static const Handle identifier = 7;
                struct Data
                {
                    CStringW value;

                    Data(void);
                    HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                    HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);

                    inline operator CStringW&()
                    {
                        return value;
                    }

                    inline operator const CStringW&() const
                    {
                        return value;
                    }


                    inline CStringW &operator = (CStringW value)
                    {
                        this->value = value;
                        return this->value;
                    }
                };
            }; // namespace Model
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek
