#pragma once

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

#define lerp(x, y, s)           (((y - x) * s) + x)

template <typename TYPE>
struct trect
{
    TYPE left;
    TYPE top;
    TYPE right;
    TYPE bottom;
};

template <typename TYPE> struct tvector2;
template <typename TYPE> struct tvector3;
template <typename TYPE> struct tvector4;
template <typename TYPE> struct tquaternion;
template <typename TYPE> struct tmatrix3x2;
template <typename TYPE> struct tmatrix4x4;

#include "Public\Vector2.h"
#include "Public\Vector3.h"
#include "Public\Vector4.h"
#include "Public\Quaternion.h"
#include "Public\Matrix3x2.h"
#include "Public\Matrix4x4.h"

typedef tvector2<float> float2;
typedef tvector3<float> float3;
typedef tvector4<float> float4;
typedef tmatrix3x2<float> float3x2;
typedef tmatrix4x4<float> float4x4;
typedef tquaternion<float> quaternion;
