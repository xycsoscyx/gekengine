#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <iostream>
#include <fstream>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#pragma warning (disable:4996)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define __thread __declspec(thread)
#define pthread_mutex_t CRITICAL_SECTION
#define pthread_mutex_init(a, b) InitializeCriticalSection(a)
#define pthread_mutex_lock(a) EnterCriticalSection(a)
#define pthread_mutex_unlock(a) LeaveCriticalSection(a)
#define pthread_mutex_destroy(a) DeleteCriticalSection(a)
#else
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#define ARRAY_SIZE(x) sizeof(x)/sizeof(x[0])

namespace Gek
{
	namespace Profiler
	{
		struct RawEvent
		{
			std::string_view name;
			std::string_view category;
			void *id;
			int64_t ts;
			uint32_t pid;
			uint32_t tid;
			char ph;
			std::string_view argumentName;
			JSON::Object argumentValue;
			double duration;
		};

		static std::vector<RawEvent> eventBuffer;
		static volatile int currentEventIndex = 0;
		static int isTracingActive = 0;
		static int64_t initializationTime = 0;
		static int isFirstLine = 1;
		static FILE *outputFile = nullptr;
		static __thread int currentThreadIdentifier;	// Thread local storage
		static pthread_mutex_t mutex;

#ifdef _WIN32
		static int GetThreadIdentifier()
		{
			return (int)GetCurrentThreadId();
		}

		static uint64_t TimerFrequency = 0;
		static uint64_t TimeStartPoint = 0;
		double GetTime()
		{
			if (TimerFrequency == 0)
			{
				QueryPerformanceFrequency((LARGE_INTEGER*)&TimerFrequency);
				QueryPerformanceCounter((LARGE_INTEGER*)&TimeStartPoint);
			}

			__int64 time;
			QueryPerformanceCounter((LARGE_INTEGER*)&time);
			return ((double)(time - TimeStartPoint) / (double)TimerFrequency);
		}

#else
		static inline int GetThreadIdentifier()
		{
			return (int)(intptr_t)pthread_self();
		}
#endif

		void Initialize(std::string_view fileName)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			eventBuffer.resize(INTERNAL_MINITRACE_BUFFER_SIZE);
			isTracingActive = 1;
			currentEventIndex = 0;
			outputFile = fopen(fileName.data(), "wb");
			std::string_view header = "{\"traceEvents\":[\n"s;
			fwrite(header.data(), 1, header.size(), outputFile);
			initializationTime = (uint64_t)(GetTime() * 1000000);
			isFirstLine = 1;
			pthread_mutex_init(&mutex, 0);
		}

		void Shutdown(void)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			isTracingActive = 0;
			Flush();
			fwrite("\n]}\n", 1, 4, outputFile);
			fclose(outputFile);
			pthread_mutex_destroy(&mutex);
			outputFile = 0;
			eventBuffer.clear();
		}

		void Resume(void)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			isTracingActive = 1;
		}

		void Pause(void)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			isTracingActive = 0;
		}

		// TODO: fwrite more than one line at a time.
		void Flush(void)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			char lineBuffer[1024];
			char argumentBuffer[256];
			char identifierBuffer[256];

			// We have to lock while flushing. So we really should avoid flushing as much as possible.
			pthread_mutex_lock(&mutex);
			int oldTracingState = isTracingActive;
			isTracingActive = 0;	// Stop logging even if using interlocked increments instead of the mutex. Can cause data loss.

			for (int eventIndex = 0; eventIndex < currentEventIndex; eventIndex++)
			{
				RawEvent *raw = &eventBuffer[eventIndex];
				int stringLength;
				snprintf(argumentBuffer, ARRAY_SIZE(argumentBuffer), "\"%s\":%s", raw->argumentName.data(), raw->argumentValue.to_string().data());
				if (raw->id)
				{
					switch (raw->ph)
					{
					case 'S':
					case 'T':
					case 'F':
						// TODO: Support full 64-bit pointers
						snprintf(identifierBuffer, ARRAY_SIZE(identifierBuffer), ",\"id\":\"0x%08x\"", (uint32_t)(uintptr_t)raw->id);
						break;

					case 'X':
						snprintf(identifierBuffer, ARRAY_SIZE(identifierBuffer), ",\"dur\":%index", (int)raw->duration);
						break;
					};
				}
				else
				{
					identifierBuffer[0] = 0;
				}

				std::string_view category = raw->category;
#ifdef _WIN32
				// On Windows, we often end up with backslashes in category.
				if (true)
				{
					char temporaryBuffer[256];
					int stringLength = std::max(category.size(), 255ULL);
					for (int categoryIndex = 0; categoryIndex < stringLength; categoryIndex++)
					{
						temporaryBuffer[categoryIndex] = category[categoryIndex] == '\\' ? '/' : category[categoryIndex];
					}

					temporaryBuffer[stringLength] = 0;
					category = temporaryBuffer;
				}
#endif

				stringLength = snprintf(lineBuffer, ARRAY_SIZE(lineBuffer), "%s{\"category\":\"%s\",\"pid\":%index,\"tid\":%index,\"ts\":%" PRId64 ",\"ph\":\"%c\",\"name\":\"%s\",\"args\":{%s}%s}",
					isFirstLine ? "" : ",\n",
					category.data(), raw->pid, raw->tid, raw->ts - initializationTime, raw->ph, raw->name.data(), argumentBuffer, identifierBuffer);
				fwrite(lineBuffer, 1, stringLength, outputFile);
				isFirstLine = 0;
			}

			currentEventIndex = 0;
			isTracingActive = oldTracingState;
			pthread_mutex_unlock(&mutex);
		}

		void AddSpan(std::string_view category, std::string_view name, double startTime, double endTime)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			if (!isTracingActive || currentEventIndex >= INTERNAL_MINITRACE_BUFFER_SIZE)
			{
				return;
		}

			double currentTime = GetTime();
			if (!currentThreadIdentifier)
			{
				currentThreadIdentifier = GetThreadIdentifier();
			}

#if 0 && _WIN32	// TODO: This needs testing
			int nextEventIndex = InterlockedIncrement(&currentEventIndex);
			RawEvent *currentEvent = &eventBuffer[nextEventIndex - 1];
#else
			pthread_mutex_lock(&mutex);
			RawEvent *currentEvent = &eventBuffer[currentEventIndex];
			currentEventIndex++;
			pthread_mutex_unlock(&mutex);
#endif

			currentEvent->category = category;
			currentEvent->name = name;
			currentEvent->ph = 'X';

			currentEvent->ts = (int64_t)(startTime * 1000000);
			currentEvent->duration = (endTime - startTime) * 1000000;

			currentEvent->tid = currentThreadIdentifier;
			currentEvent->pid = 0;
		}

		void AddEvent(std::string_view category, std::string_view name, char ph, void *id)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			if (!isTracingActive || currentEventIndex >= INTERNAL_MINITRACE_BUFFER_SIZE)
			{
				return;
			}

			double currentTime = GetTime();
			if (!currentThreadIdentifier)
			{
				currentThreadIdentifier = GetThreadIdentifier();
			}

#if 0 && _WIN32	// TODO: This needs testing
			int nextEventIndex = InterlockedIncrement(&currentEventIndex);
			RawEvent *currentEvent = &eventBuffer[nextEventIndex - 1];
#else
			pthread_mutex_lock(&mutex);
			RawEvent *currentEvent = &eventBuffer[currentEventIndex];
			currentEventIndex++;
			pthread_mutex_unlock(&mutex);
#endif

			currentEvent->category = category;
			currentEvent->name = name;
			currentEvent->id = id;
			currentEvent->ph = ph;
			if (currentEvent->ph == 'X')
			{
				double x;
				memcpy(&x, id, sizeof(double));
				currentEvent->ts = (int64_t)(x * 1000000);
				currentEvent->duration = (currentTime - x) * 1000000;
			}
			else
			{
				currentEvent->ts = (int64_t)(currentTime * 1000000);
			}

			currentEvent->tid = currentThreadIdentifier;
			currentEvent->pid = 0;
		}

		void AddEvent(std::string_view category, std::string_view name, char ph, void *id, std::string_view argumentName, JSON::Reference argumentValue)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			if (!isTracingActive || currentEventIndex >= INTERNAL_MINITRACE_BUFFER_SIZE)
			{
				return;
			}

			if (!currentThreadIdentifier)
			{
				currentThreadIdentifier = GetThreadIdentifier();
			}

			double ts = GetTime();

#if 0 && _WIN32	// TODO: This needs testing
			int nextEventIndex = InterlockedIncrement(&currentEventIndex);
			RawEvent *currentEvent = &eventBuffer[nextEventIndex - 1];
#else
			pthread_mutex_lock(&mutex);
			RawEvent *currentEvent = &eventBuffer[currentEventIndex];
			currentEventIndex++;
			pthread_mutex_unlock(&mutex);
#endif

			currentEvent->category = category;
			currentEvent->name = name;
			currentEvent->id = id;
			currentEvent->ts = (int64_t)(ts * 1000000);
			currentEvent->ph = ph;
			currentEvent->tid = currentThreadIdentifier;
			currentEvent->pid = 0;
			currentEvent->argumentName = argumentName;
			currentEvent->argumentValue = argumentValue.getObject();
		}
	}; // namespace Profiler
}; // namespace Gek
