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
#ifdef GEK_ENABLE_PROFILER
    #define GEK_PROFILE_REGISTER_NAME(PROFILER, NAME) PROFILER->registerName(NAME)
    #define GEK_PROFILE_REGISTER_THREAD(PROFILER, NAME) PROFILER->registerThreadName(NAME)
    #define GEK_PROFILE_BEGIN_FRAME(PROFILER) PROFILER->beginFrame()
    #define GEK_PROFILE_END_FRAME(PROFILER) PROFILER->endFrame()
    #define GEK_PROFILE_EVENT(PROFILER, NAME, THREAD, START_TIME, END_TIME) PROFILER->addTimeStamp(Gek::System::Profiler::TimeStamp(NAME, START_TIME, END_TIME, &THREAD))

    #define GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME) \
            static const auto __hash##NAME__ = PROFILER->registerName(NAME); \
            Gek::System::Profiler::ScopedEvent __event##NAME__(PROFILER, __hash##NAME__);

    #define GEK_PROFILE_FUNCTION(PROFILER) GEK_PROFILE_AUTO_SCOPE(PROFILER, __FUNCTION__)

    #define GEK_PROFILE_BEGIN_SCOPE(PROFILER, NAME) \
            [&](void) -> void \
            { \
                GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME)

    #define GEK_PROFILE_END_SCOPE() \
            }()
#else
    #define GEK_PROFILE_REGISTER_NAME(PROFILER, NAME) 0
    #define GEK_PROFILE_REGISTER_THREAD(PROFILER, NAME)
    #define GEK_PROFILE_BEGIN_FRAME(PROFILER)
    #define GEK_PROFILE_END_FRAME(PROFILER)
    #define GEK_PROFILE_EVENT(PROFILER, NAME, THREAD, START_TIME, END_TIME)
    #define GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME)
    #define GEK_PROFILE_FUNCTION(PROFILER)
    #define GEK_PROFILE_BEGIN_SCOPE(PROFILER, NAME) \
        [&](void) -> void \
        { \

    #define GEK_PROFILE_END_SCOPE() \
        }()
#endif

namespace Gek
{
    namespace System
    {
        class Profiler
        {
        public:
            struct TimeStamp
            {
                enum class Type
                {
                    Begin = 0,
                    End,
                    Duration,
                };

                Type type = Type::Begin;
                Hash nameIdentifier = 0;
                Hash threadIdentifier = 0;
                std::chrono::nanoseconds timeStamp, duration;

                TimeStamp(void) = default;
                TimeStamp(Type type, Hash nameIdentifier, std::chrono::nanoseconds const *timeStamp = nullptr, Hash const *threadIdentifier = nullptr);
                TimeStamp(Hash nameIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime, Hash const *threadIdentifier = nullptr);
            };

            struct ScopedEvent
            {
                Hash nameIdentifier = 0;
                Profiler *profiler = nullptr;
                std::chrono::nanoseconds timeStamp;

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

            std::chrono::nanoseconds frameTimeStamp;
            concurrency::concurrent_unordered_map<Hash, std::string> nameMap;
            concurrency::concurrent_vector<TimeStamp> timeStampList;

        private:
            void writeTimeStamp(TimeStamp *timeStamp);
            void flushTimeStampList(concurrency::concurrent_vector<TimeStamp> &&timeStampList);

        public:
            Profiler(void);
            virtual ~Profiler(void);

            Hash registerName(std::string_view name);
            void registerThreadName(std::string_view name);

            void beginFrame(void);
            void addTimeStamp(TimeStamp const &flushList);
            void endFrame(void);
        };
    }; // namespace System
}; // namespace Gek
