#pragma once
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace Gek
{
    namespace Engine
    {
        inline void* LoadPlugin(const std::string& path) {
        #ifdef _WIN32
            #ifdef _DEBUG
                return reinterpret_cast<void*>(LoadLibraryA((path + "_debug.dll").c_str()));
            #else
                return reinterpret_cast<void*>(LoadLibraryA((path + ".dll").c_str()));
            #endif
        #else   
            #ifdef _DEBUG
                return dlopen((path + "_debug.so").c_str(), RTLD_LAZY);
            #else
                return dlopen((path + ".so").c_str(), RTLD_LAZY);
            #endif
        #endif
        }
    } // namespace Engine
} // namespace Gek