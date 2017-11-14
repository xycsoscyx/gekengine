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
    #define GEK_PROFILE_BEGIN_FRAME(PROFILER) PROFILER->beginFrame()
    #define GEK_PROFILE_END_FRAME(PROFILER) PROFILER->endFrame()
    #define GEK_PROFILE_EVENT(PROFILER, TYPE, NAME, THREAD, TIME) PROFILER->addTimeStamp(Gek::Profiler::TimeStamp(Gek::Profiler::TimeStamp::Type::TYPE, NAME, &TIME, &THREAD))

    #define GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME) \
        static const auto __hash##NAME__ = PROFILER->registerName(NAME); \
        Gek::Profiler::ScopedEvent __event##NAME__(PROFILER, __hash##NAME__);

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
        struct TimeStamp
        {
            enum class Type
            {
                Begin = 0,
                End,
            };

            Type type = Type::Begin;
            Hash nameIdentifier = 0;
            Hash threadIdentifier = 0;
            std::chrono::nanoseconds timeStamp;

            TimeStamp(void) = default;
            TimeStamp(Type type, Hash nameIdentifier, std::chrono::nanoseconds const *timeStamp = nullptr, Hash const *threadIdentifier = nullptr);
        };

        struct ScopedEvent
        {
            Hash nameIdentifier = 0;
            Profiler *profiler = nullptr;

            ScopedEvent(void) = default;
            ScopedEvent(Profiler *profiler, uint64_t nameIdentifier);
            ~ScopedEvent(void);
        };

    private:
        uint64_t processIdentifier = 0;
        std::chrono::high_resolution_clock clock;
        concurrency::critical_section criticalSection;
        Hash profilerName = 0;
        Hash mainThread = 0;

        ThreadPool<1> writePool;
        std::ofstream fileOutput;

        concurrency::concurrent_unordered_map<Hash, std::string> nameMap;
        concurrency::concurrent_vector<TimeStamp> timeStampList;

    private:
        void writeTimeStamp(TimeStamp *timeStamp);
        void flushTimeStampList(concurrency::concurrent_vector<TimeStamp> &&timeStampList);

    public:
        Profiler(void);
        virtual ~Profiler(void);

        Hash registerName(const char* const name);
        void registerThreadName(const char* const name);

        void beginFrame(void);
        void addTimeStamp(TimeStamp const &flushList);
        void endFrame(void);
    };
#else
    #define GEK_PROFILE_REGISTER_NAME(PROFILER, NAME) 0
    #define GEK_PROFILE_REGISTER_THREAD(PROFILER, NAME)
    #define GEK_PROFILE_BEGIN_FRAME(PROFILER)
    #define GEK_PROFILE_END_FRAME(PROFILER)
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
