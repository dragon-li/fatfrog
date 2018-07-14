/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef LIYL_THREAD_DEFS_H
#define LIYL_THREAD_DEFS_H


#if defined(__cplusplus)
extern "C" {
#endif

enum {
    /*
     * ***********************************************
     * ** Keep in sync with android.os.Process.java **
     * ***********************************************
     *
     * This maps directly to the "nice" priorities we use in Android.
     * A thread priority should be chosen inverse-proportionally to
     * the amount of work the thread is expected to do. The more work
     * a thread will do, the less favorable priority it should get so that
     * it doesn't starve the system. Threads not behaving properly might
     * be "punished" by the kernel.
     * Use the levels below when appropriate. Intermediate values are
     * acceptable, preferably use the {MORE|LESS}_FAVORABLE constants below.
     */
    LIYL_PRIORITY_LOWEST         =  19,

    /* use for background tasks */
    LIYL_PRIORITY_BACKGROUND     =  10,

    /* most threads run at normal priority */
    LIYL_PRIORITY_NORMAL         =   0,

    /* threads currently running a UI that the user is interacting with */
    LIYL_PRIORITY_FOREGROUND     =  -2,

    /* the main UI thread has a slightly more favorable priority */
    LIYL_PRIORITY_DISPLAY        =  -4,

    /* ui service treads might want to run at a urgent display (uncommon) */
    LIYL_PRIORITY_URGENT_DISPLAY =  -8/*HAL_PRIORITY_URGENT_DISPLAY*/,

    /* all normal audio threads */
    LIYL_PRIORITY_AUDIO          = -16,

    /* service audio threads (uncommon) */
    LIYL_PRIORITY_URGENT_AUDIO   = -19,

    /* should never be used in practice. regular process might not
     * be allowed to use this level */
    LIYL_PRIORITY_HIGHEST        = -20,

    LIYL_PRIORITY_DEFAULT        = LIYL_PRIORITY_NORMAL,
    LIYL_PRIORITY_MORE_FAVORABLE = -1,
    LIYL_PRIORITY_LESS_FAVORABLE = +1,
};

#if defined(__cplusplus)
}
#endif

#endif /* LIYL_THREAD_DEFS_H */
