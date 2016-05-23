#pragma once

#include "GEK\Context\ContextUser.h"
#include "GEK\Utility\Hash.h"
#include <assert.h>
#include <atlbase.h>
#include <unordered_map>
#include <functional>
#include <memory>

#define REGISTER_CLASS(CLASS)                                                                       \
std::shared_ptr<Gek::ContextUser> CLASS##CreateInstance(void *parameters)                           \
{                                                                                                   \
    return std::dynamic_pointer_cast<Gek::ContextUser>(std::make_shared<CLASS>());                  \
}

#define DECLARE_REGISTERED_CLASS(CLASS)                                                             \
extern std::shared_ptr<Gek::ContextUser> CLASS##CreateInstance(void *parameters);

#define DECLARE_PLUGIN_MAP(SOURCENAME)                                                              \
extern "C" __declspec(dllexport)                                                                    \
void GEKGetModuleClasses(                                                                           \
    std::unordered_map<CStringW, std::function<std::shared_ptr<Gek::ContextUser>(void)>> &classList,\
    std::unordered_map<CStringW, std::vector<CStringW>> &typedClassList)                            \
{                                                                                                   \
    CStringW lastClassName;

#define ADD_PLUGIN_CLASS(CLASSNAME, CLASS)                                                          \
    if (classList.find(CLASSNAME) == classList.end())                                               \
    {                                                                                               \
        classList[CLASSNAME] = CLASS##CreateInstance;                                               \
        lastClassName = CLASSNAME;                                                                  \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        _ASSERTE(!"Duplicate class found in module: " #CLASSNAME);                                  \
    }

#define ADD_PLUGIN_CLASS_TYPE(TYPEID)                                                               \
    typedClassList[TYPEID].push_back(lastClassName);

#define END_PLUGIN_MAP                                                                              \
}
