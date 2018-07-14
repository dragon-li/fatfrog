/*
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "SchedPolicy"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sched_policy.h"
#include "log.h"

#define UNUSED __attribute__((__unused__))

/* Re-map SP_DEFAULT to the system default policy, and leave other values unchanged.
 * Call this any place a SchedPolicy is used as an input parameter.
 * Returns the possibly re-mapped policy.
 */
static inline SchedPolicy _policy(SchedPolicy p)
{
   return p == SP_DEFAULT ? SP_SYSTEM_DEFAULT : p;
}

#if defined(__ANDROID__)

#include <pthread.h>
#include <sched.h>
#include <sys/prctl.h>

/* Add tid to the scheduling group defined by the policy */
static int add_tid_to_cgroup(int tid, int fd)
{
    //TODO
    errno = ENOSYS;
    return -1;
}

static void __initialize(void) {
    //TODO
    errno = ENOSYS;
    return;
}

static int getCGroupSubsys(int tid, const char* subsys, char* buf, size_t bufLen)
{

    //TODO
    errno = ENOSYS;
    return -1;
}

int get_sched_policy(int tid, SchedPolicy *policy)
{
    //TODO
    errno = ENOSYS;
    return -1;
}

int set_cpuset_policy(int tid, SchedPolicy policy)
{
    //TODO
    errno = ENOSYS;
    return -1;
}

int set_sched_policy(int tid, SchedPolicy policy)
{
    //TODO
    errno = ENOSYS;
    return -1;
}

#else

/* Stubs for non-Android targets. */

int set_sched_policy(int tid UNUSED, SchedPolicy policy UNUSED)
{
    return 0;
}

int get_sched_policy(int tid UNUSED, SchedPolicy *policy)
{
    *policy = SP_SYSTEM_DEFAULT;
    return 0;
}

#endif

const char *get_sched_policy_name(SchedPolicy policy)
{
    policy = _policy(policy);
    static const char * const strings[SP_CNT] = {
       [SP_BACKGROUND] = "bg",
       [SP_FOREGROUND] = "fg",
       [SP_SYSTEM]     = "  ",
       [SP_AUDIO_APP]  = "aa",
       [SP_AUDIO_SYS]  = "as",
       [SP_TOP_APP]    = "ta",
    };
    if ((policy < SP_CNT) && (strings[policy] != NULL))
        return strings[policy];
    else
        return "error";
}
