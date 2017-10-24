/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <fstream>
#include <chrono>

#define GEK_ENABLE_PROFILER

namespace Gek
{
#ifdef GEK_ENABLE_PROFILER
    #define GEK_PROFILE_REGISTER_NAME(PROFILER, NAME) PROFILER->registerName(NAME)
    #define GEK_PROFILE_REGISTER_THREAD(PROFILER, NAME) PROFILER->registerThreadName(NAME)
    #define GEK_PROFILE_EVENT(PROFILER, NAME, THREAD, START, END) PROFILER->addEvent(Gek::Profiler::Data(NAME, THREAD, START, END))

    #define GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME) \
        static const auto __hash##NAME__ = PROFILER->registerName(NAME); \
        Gek::Profiler::Event __event##NAME__(PROFILER, __hash##NAME__);

    #define GEK_PROFILE_FUNCTION(PROFILER) GEK_PROFILE_AUTO_SCOPE(PROFILER, __FUNCTION__)

    #define GEK_PROFILE_BEGIN_SCOPE(PROFILER, NAME) \
        [&](void) -> void \
        { \
            GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME)

    #define GEK_PROFILE_END_SCOPE() \
        }()

    class Profiler
    {
    public:
        struct Data
        {
            Hash nameHash = 0;
            Hash threadIdentifier;
            std::chrono::nanoseconds startTime;
            std::chrono::nanoseconds endTime;

            Data(void) = default;
            Data(Hash nameHash);
            Data(Hash nameHash, Hash threadIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime);
        };

        struct Event
            : public Data
        {
            Event(void) = default;
            Event(Profiler *profiler, uint64_t nameHash);
            ~Event(void);

            Profiler *profiler = nullptr;
        };

    private:
        uint64_t processIdentifier = 0;
        std::chrono::high_resolution_clock clock;
        concurrency::critical_section criticalSection;

        ThreadPool<1> writePool;
        std::ofstream fileOutput;

        concurrency::concurrent_unordered_map<Hash, std::string> nameMap;
        concurrency::concurrent_vector<Data> buffer;

    private:
        void flushQueue(void);

    public:
        Profiler(void);
        virtual ~Profiler(void);

        Hash registerName(const char* const name);
        void registerThreadName(const char* const name);
        void addEvent(Data const &data);
    };
#else
    #define GEK_PROFILE_REGISTER_NAME(PROFILER, NAME) 0
    #define GEK_PROFILE_REGISTER_THREAD(PROFILER, NAME)
    #define GEK_PROFILE_EVENT(PROFILER, NAME, THREAD, START, END)
    #define GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME)
    #define GEK_PROFILE_FUNCTION(PROFILER)
    #define GEK_PROFILE_BEGIN_SCOPE(PROFILER, NAME) \
        [&](void) -> void \
        { \

    #define GEK_PROFILE_END_SCOPE() \
        }()

    struct Profiler
    {
        virtual ~Profiler(void) = default;
    };
#endif
}; // namespace Gek
