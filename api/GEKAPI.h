#pragma once

// Interfaces
#include "IGEKComponent.h"
#include "IGEKMaterialManager.h"
#include "IGEKProgramManager.h"
#include "IGEKSceneManager.h"
#include "IGEKRenderManager.h"
#include "IGEKInputManager.h"

// Global
enum GEKVERTEXATTRIBUTES
{
    GEK_VERTEX_POSITION         = 1 << 0,
    GEK_VERTEX_TEXCOORD         = 1 << 1,
    GEK_VERTEX_NORMAL           = 1 << 2,
};