#pragma once

#include <xmmintrin.h>
#include <ostream>

#ifndef max
#define max(x, y)   ((x) > (y) ? (x) : (y))
#endif

#ifndef min
#define min(x, y)   ((x) < (y) ? (x) : (y))
#endif

static union 
{
    unsigned char c[4]; 
    float f; 
} _HUGEVALUE = {{0,0,0x80,0x7f}};

const float _INFINITY           = _HUGEVALUE.f;
const float _EPSILON            = 1.0e-5f;
const float _PI                 = 3.14159265358979323846f;
const float _2_PI               = 6.28318530717958623200f;
const float _PI_2               = 1.57079632679489661923f;
const float _INV_PI             = 0.31830988618379069122f;

#define _DEGTORAD(x)            ((x) * (_PI / 180.0f))
#define _RADTODEG(x)            ((x) * (180.0f / _PI))

#include "Public\Vector2.h"
#include "Public\Vector3.h"
#include "Public\Vector4.h"
#include "Public\Quaternion.h"
#include "Public\Matrix4x4.h"
#include "Public\Plane.h"
#include "Public\Sphere.h"
#include "Public\AABB.h"
#include "Public\OBB.h"
#include "Public\Ray.h"
#include "Public\Frustum.h"

typedef tvector2<float> float2;
typedef tvector3<float> float3;
typedef tvector4<float> float4;
typedef tmatrix4x4<float> float4x4;
typedef tquaternion<float> quaternion;
typedef tsphere<float> sphere;
typedef taabb<float> aabb;
typedef tobb<float> obb;
typedef tray<float> ray;
typedef tplane<float> plane;
typedef tfrustum<float> frustum;
