#pragma once

// Utilities
#include "GEKVALUE.h"

// Interfaces
#include "IGEKResource.h"
#include "IGEKEntity.h"
#include "IGEKLogic.h"
#include "IGEKComponent.h"
#include "IGEKComponentSystem.h"
#include "IGEKProgramManager.h"
#include "IGEKModel.h"
#include "IGEKModelManager.h"
#include "IGEKFactory.h"
#include "IGEKMaterialManager.h"
#include "IGEKSceneManager.h"
#include "IGEKViewManager.h"

// Global
enum GEKVERTEXATTRIBUTES
{
    GEK_VERTEX_POSITION         = 1 << 0,
    GEK_VERTEX_TEXCOORD         = 1 << 1,
    GEK_VERTEX_BASIS            = 1 << 2,
};