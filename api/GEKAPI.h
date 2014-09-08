#pragma once

// Utilities
#include "GEKVALUE.h"

// Interfaces
#include "IGEKResource.h"
#include "IGEKFactory.h"

#include "IGEKComponent.h"

#include "IGEKMaterialManager.h"
#include "IGEKProgramManager.h"
#include "IGEKModelManager.h"
#include "IGEKSceneManager.h"
#include "IGEKRenderManager.h"
#include "IGEKInputManager.h"

// Global
enum GEKVERTEXATTRIBUTES
{
    GEK_VERTEX_POSITION         = 1 << 0,
    GEK_VERTEX_TEXCOORD         = 1 << 1,
    GEK_VERTEX_BASIS            = 1 << 2,
};