/*
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "libc_init_common.h"

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "private/bionic_globals.h"
#include "private/WriteProtected.h"


__LIBC_HIDDEN__ WriteProtected<liyl_libc_globals> my_libc_globals;


void liyl_libc_init_globals() {
    // Initialize libc globals that are needed in both the linker and in libc.
  // In dynamic binaries, this is run at least twice for different copies of the
  // globals, once for the linker's copy and once for the one in libc.so.
    my_libc_globals.initialize();
    my_libc_globals.mutate([](liyl_libc_globals* globals) {
            globals->needCallWhenExit = true;
            });
}

void liyl_globals_preinit() {
    // Initialize libc globals
    my_libc_globals.initialize();
    my_libc_globals.mutate([](liyl_libc_globals* globals) {
            globals->needCallWhenExit = false;
            });
    my_libc_globals.mutate(liyl_libc_init_malloc);

}

void liyl_globals_deinit() {

    my_libc_globals.mutate(liyl_libc_fini_malloc);

}
