#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Engine\Component.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct CameraComponent
    {
        enum class Mode : UINT8
        {
            Perspective = 0,
            Orthographic,
        };

        Mode mode;
        float minimumDistance;
        float maximumDistance;
        union
        {
            float size; // Orthographic
            float fieldOfView; // Perspective
        };

        CameraComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
    };

    struct FirstPersonCameraComponent : public CameraComponent
    {
        FirstPersonCameraComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
    };

    struct ThirdPersonCameraComponent : public CameraComponent
    {
        CStringW body;
        Math::Float3 offset;
        Math::Float3 distance;

        ThirdPersonCameraComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
    };
}; // namespace Gek
