#pragma once

#include "GEKMath.h"

#include "Public\Plane.h"
#include "Public\Sphere.h"
#include "Public\AABB.h"
#include "Public\OBB.h"
#include "Public\Ray.h"
#include "Public\Frustum.h"

typedef tplane<float> plane;
typedef tsphere<float> sphere;
typedef taabb<float> aabb;
typedef tobb<float> obb;
typedef tray<float> ray;
typedef tfrustum<float> frustum;
/*
#include <Eigen\Core>
#include <Eigen\Geometry>

#include "Include\GEKPlane.h"
#include "Include\GEKSphere.h"
#include "Include\GEKAABB.h"
#include "Include\GEKOBB.h"
#include "Include\GEKRay.h"
#include "Include\GEKFrustum.h"
*/