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
		// Ugh, this struct is already pretty heavy.
		// Will probably need to move arguments to a second buffer to support more than one.
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

		static std::vector<RawEvent> buffer;
		static volatile int count = 0;
		static int is_tracing = 0;
		static int64_t time_offset = 0;
		static int first_line = 1;
		static FILE *file = nullptr;
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
			buffer.resize(INTERNAL_MINITRACE_BUFFER_SIZE);
			is_tracing = 1;
			count = 0;
			file = fopen(fileName.data(), "wb");
			std::string_view header = "{\"traceEvents\":[\n"s;
			fwrite(header.data(), 1, header.size(), file);
			time_offset = (uint64_t)(GetTime() * 1000000);
			first_line = 1;
			pthread_mutex_init(&mutex, 0);
		}

		void Shutdown(void)
		{
			int i;
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			is_tracing = 0;
			Flush();
			fwrite("\n]}\n", 1, 4, file);
			fclose(file);
			pthread_mutex_destroy(&mutex);
			file = 0;
			buffer.clear();
		}

		void Resume(void)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			is_tracing = 1;
		}

		void Pause(void)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			is_tracing = 0;
		}

		// TODO: fwrite more than one line at a time.
		void Flush(void)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			int i = 0;
			char linebuf[1024];
			char arg_buf[256];
			char id_buf[256];

			// We have to lock while flushing. So we really should avoid flushing as much as possible.
			pthread_mutex_lock(&mutex);
			int old_tracing = is_tracing;
			is_tracing = 0;	// Stop logging even if using interlocked increments instead of the mutex. Can cause data loss.

			for (i = 0; i < count; i++)
			{
				RawEvent *raw = &buffer[i];
				int len;
				snprintf(arg_buf, ARRAY_SIZE(arg_buf), "\"%s\":%s", raw->argumentName, raw->argumentValue.to_string().data());
				if (raw->id)
				{
					switch (raw->ph)
					{
					case 'S':
					case 'T':
					case 'F':
						// TODO: Support full 64-bit pointers
						snprintf(id_buf, ARRAY_SIZE(id_buf), ",\"id\":\"0x%08x\"", (uint32_t)(uintptr_t)raw->id);
						break;

					case 'X':
						snprintf(id_buf, ARRAY_SIZE(id_buf), ",\"dur\":%i", (int)raw->duration);
						break;
					}
				}
				else
				{
					id_buf[0] = 0;
				}

				std::string_view category = raw->category;
#ifdef _WIN32
				// On Windows, we often end up with backslashes in category.
				if (true)
				{
					char temp[256];
					int len = (int)strlen(category);
					int i;
					if (len > 255) len = 255;
					for (i = 0; i < len; i++)
					{
						temp[i] = category[i] == '\\' ? '/' : category[i];
					}

					temp[len] = 0;
					category = temp;
				}
#endif

				len = snprintf(linebuf, ARRAY_SIZE(linebuf), "%s{\"category\":\"%s\",\"pid\":%i,\"tid\":%i,\"ts\":%" PRId64 ",\"ph\":\"%c\",\"name\":\"%s\",\"args\":{%s}%s}",
					first_line ? "" : ",\n",
					category, raw->pid, raw->tid, raw->ts - time_offset, raw->ph, raw->name, arg_buf, id_buf);
				fwrite(linebuf, 1, len, file);
				first_line = 0;
			}

			count = 0;
			is_tracing = old_tracing;
			pthread_mutex_unlock(&mutex);
		}

		void AddEvent(std::string_view category, std::string_view name, char ph, void *id)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			if (!is_tracing || count >= INTERNAL_MINITRACE_BUFFER_SIZE)
			{
				return;
			}

			double ts = GetTime();
			if (!currentThreadIdentifier)
			{
				currentThreadIdentifier = GetThreadIdentifier();
			}

#if 0 && _WIN32	// TODO: This needs testing
			int bufPos = InterlockedIncrement(&count);
			RawEvent *ev = &buffer[count - 1];
#else
			pthread_mutex_lock(&mutex);
			RawEvent *ev = &buffer[count];
			count++;
			pthread_mutex_unlock(&mutex);
#endif

			ev->category = category;
			ev->name = name;
			ev->id = id;
			ev->ph = ph;
			if (ev->ph == 'X')
			{
				double x;
				memcpy(&x, id, sizeof(double));
				ev->ts = (int64_t)(x * 1000000);
				ev->duration = (ts - x) * 1000000;
			}
			else
			{
				ev->ts = (int64_t)(ts * 1000000);
			}

			ev->tid = currentThreadIdentifier;
			ev->pid = 0;
		}

		void AddEvent(std::string_view category, std::string_view name, char ph, void *id, std::string_view argumentName, JSON::Reference argumentValue)
		{
#ifndef GEK_PROFILER_ENABLED
			return;
#endif
			if (!is_tracing || count >= INTERNAL_MINITRACE_BUFFER_SIZE)
			{
				return;
			}

			if (!currentThreadIdentifier)
			{
				currentThreadIdentifier = GetThreadIdentifier();
			}

			double ts = GetTime();

#if 0 && _WIN32	// TODO: This needs testing
			int bufPos = InterlockedIncrement(&count);
			RawEvent *ev = &buffer[count - 1];
#else
			pthread_mutex_lock(&mutex);
			RawEvent *ev = &buffer[count];
			count++;
			pthread_mutex_unlock(&mutex);
#endif

			ev->category = category;
			ev->name = name;
			ev->id = id;
			ev->ts = (int64_t)(ts * 1000000);
			ev->ph = ph;
			ev->tid = currentThreadIdentifier;
			ev->pid = 0;
			ev->argumentName = argumentName;
			ev->argumentValue = argumentValue.getObject();
		}
	}; // namespace Profiler
}; // namespace Gek
