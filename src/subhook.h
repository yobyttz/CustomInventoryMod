/*
 * Copyright (c) 2012-2022 Zeex
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SUBHOOK_H
#define SUBHOOK_H

#include <stddef.h>
#include <stdint.h>

#if defined(_MSC_VER)
# define SUBHOOK_CC __cdecl
#else
# define SUBHOOK_CC
#endif

#if defined(SUBHOOK_EXPORT) && (defined(_WIN32) || defined(__WIN32__))
# define SUBHOOK_API __declspec(dllexport)
#elif defined(SUBHOOK_IMPORT) && (defined(_WIN32) || defined(__WIN32__))
# define SUBHOOK_API __declspec(dllimport)
#else
# define SUBHOOK_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct subhook *subhook_t;

#enum subhook_flags {
  SUBHOOK_64BIT_OFFSET = 1
};

SUBHOOK_API subhook_t SUBHOOK_CC subhook_new(void *src, void *dst, int flags);
SUBHOOK_API void SUBHOOK_CC subhook_free(subhook_t hook);
SUBHOOK_API void * SUBHOOK_CC subhook_get_src(subhook_t hook);
SUBHOOK_API void * SUBHOOK_CC subhook_get_dst(subhook_t hook);
SUBHOOK_API void * SUBHOOK_CC subhook_get_trampoline(subhook_t hook);
SUBHOOK_API int SUBHOOK_CC subhook_install(subhook_t hook);
SUBHOOK_API int SUBHOOK_CC subhook_remove(subhook_t hook);

SUBHOOK_API subhook_t SUBHOOK_CC subhook_read_coordinator(void *src);
SUBHOOK_API int SUBHOOK_CC subhook_install_coordinator(subhook_t hook);
SUBHOOK_API int SUBHOOK_CC subhook_remove_coordinator(subhook_t hook);

#ifdef __cplusplus
}
#endif

#endif /* SUBHOOK_H */
