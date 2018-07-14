/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef LIYL_LIBC_BIONIC_DLMALLOC_H_
#define LIYL_LIBC_BIONIC_DLMALLOC_H_

#include <sys/cdefs.h>
#include <stddef.h>

/* Configure dlmalloc. */
#include "dlconfig.h"

//TODO FIXME
//#if !defined(__LP64__)
///* dlmalloc_usable_size and dlmalloc were exposed in the NDK and some
// * apps actually used them. Rename these functions out of the way
// * for 32 bit architectures so that ndk_cruft.cpp can expose
// * compatibility shims with these names.
// */
//#define dlmalloc_usable_size dlmalloc_usable_size_real
//#define dlmalloc dlmalloc_real
//#endif

/* Export two symbols used by the VM. */
__BEGIN_DECLS
#undef __LIBC_ABI_PUBLIC__
#define __LIBC_ABI_PUBLIC__  __attribute__((visibility ("default")))
int dlmalloc_trim(size_t) __LIBC_ABI_PUBLIC__;
void dlmalloc_inspect_all(void (*handler)(void*, void*, size_t, void*), void*) __LIBC_ABI_PUBLIC__;
__END_DECLS

/* Include the proper definitions. */
#include "mymalloc.h"

#endif  // LIBC_BIONIC_DLMALLOC_H_
