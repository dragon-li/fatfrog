/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _UTILS_LIYL_MUTEX_H
#define _UTILS_LIYL_MUTEX_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include <pthread.h>

#include "Errors.h"
#include "Timers.h"

// ---------------------------------------------------------------------------
LIYL_NAMESPACE_START
// ---------------------------------------------------------------------------

class Condition;

/*
 * NOTE: This class is for code that builds on Win32.  Its usage is
 * deprecated for code which doesn't build for Win32.  New code which
 * doesn't build for Win32 should use std::mutex and std::lock_guard instead.
 *
 * Simple mutex class.  The implementation is system-dependent.
 *
 * The mutex must be unlocked by the thread that locked it.  They are not
 * recursive, i.e. the same thread can't lock it multiple times.
 */
class Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

                Mutex();
                Mutex(const char* name);
                Mutex(int type, const char* name = NULL);
                ~Mutex();

    // lock or unlock the mutex
    status_t    lock();
    void        unlock();

    // lock if possible; returns 0 on success, error otherwise
    status_t    tryLock();

#if defined(__ANDROID__)
    // Lock the mutex, but don't wait longer than timeoutNs (relative time).
    // Returns 0 on success, TIMED_OUT for failure due to timeout expiration.
    //
    // OSX doesn't have pthread_mutex_timedlock() or equivalent. To keep
    // capabilities consistent across host OSes, this method is only available
    // when building Android binaries.
    //
    // FIXME?: pthread_mutex_timedlock is based on CLOCK_REALTIME,
    // which is subject to NTP adjustments, and includes time during suspend,
    // so a timeout may occur even though no processes could run.
    // Not holding a partial wakelock may lead to a system suspend.
    /*LIYL_ADD Don't support ,pthread_mutex_timedlock need android 7.0 && ndk r15 */
    //status_t    timedLock(nsecs_t timeoutNs);
#endif

    // Manages the mutex automatically. It'll be locked when Autolock is
    // constructed and released when Autolock goes out of scope.
    class Autolock {
    public:
        inline Autolock(Mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };

private:
    friend class Condition;

    // A mutex cannot be copied
                Mutex(const Mutex&);
    Mutex&      operator = (const Mutex&);

    pthread_mutex_t mMutex;
};

// ---------------------------------------------------------------------------


inline Mutex::Mutex() {
    pthread_mutex_init(&mMutex, NULL);
}
inline Mutex::Mutex(__attribute__((unused)) const char* name) {
    pthread_mutex_init(&mMutex, NULL);
}
inline Mutex::Mutex(int type, __attribute__((unused)) const char* name) {
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, NULL);
    }
}
inline Mutex::~Mutex() {
    pthread_mutex_destroy(&mMutex);
}
inline status_t Mutex::lock() {
    return -pthread_mutex_lock(&mMutex);
}
inline void Mutex::unlock() {
    pthread_mutex_unlock(&mMutex);
}
inline status_t Mutex::tryLock() {
    return -pthread_mutex_trylock(&mMutex);
}
#if defined(__ANDROID__)
  /*LIYL_ADD Don't support ,pthread_mutex_timedlock need android 7.0 && ndk r15 */
//inline status_t Mutex::timedLock(nsecs_t timeoutNs) {
//    timeoutNs += systemTime(SYSTEM_TIME_REALTIME);
//    const struct timespec ts = {
//        /* .tv_sec = */ static_cast<time_t>(timeoutNs / 1000000000),
//        /* .tv_nsec = */ static_cast<long>(timeoutNs % 1000000000),
//    };
//    return -pthread_mutex_timedlock(&mMutex, &ts);
//}
#endif


// ---------------------------------------------------------------------------

/*
 * Automatic mutex.  Declare one of these at the top of a function.
 * When the function returns, it will go out of scope, and release the
 * mutex.
 */

typedef Mutex::Autolock AutoMutex;

// ---------------------------------------------------------------------------
LIYL_NAMESPACE_END
// ---------------------------------------------------------------------------

#endif // _UTILS_LIYL_MUTEX_H
