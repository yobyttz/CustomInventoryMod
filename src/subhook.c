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
 * INTERRUPTION) HOWEVER CAUSED AND ONANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(__WIN32__)
# include <windows.h>
#else
# include <sys/mman.h>
# include <unistd.h>
#endif

#include "subhook.h"

#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
# define SUBHOOK_X86
#elif defined(__aarch64__) || defined(_M_ARM64)
# define SUBHOOK_ARM64
#elif defined(__arm__) || defined(_M_ARM)
# define SUBHOOK_ARM
#endif

#if defined(SUBHOOK_X86)
# if defined(__x86_64__) || defined(_M_X64)
#  define SUBHOOK_X86_64
# else
#  define SUBHOOK_X86_32
# endif
#endif

struct subhook {
  void *src;
  void *dst;
  int flags;
  int installed;
  void *trampoline;
  unsigned char original_code[16];
  size_t patch_size;
};

#if defined(_WIN32) || defined(__WIN32__)
static int subhook_unprotect(void *address, size_t size) {
  MEMORY_BASIC_INFORMATION mbi;
  if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0)
    return -1;
  return VirtualProtect(mbi.BaseAddress, size, PAGE_EXECUTE_READWRITE, &mbi.Protect) ? 0 : -1;
}

static int subhook_protect(void *address, size_t size) {
  MEMORY_BASIC_INFORMATION mbi;
  if (VirtualQuery(address, &mbi, sizeof(mbi)) == 0)
    return -1;
  DWORD old_protect;
  return VirtualProtect(mbi.BaseAddress, size, mbi.Protect, &old_protect) ? 0 : -1;
}
#else
static size_t subhook_page_size(void) {
  return sysconf(_SC_PAGESIZE);
}

static void *subhook_page_align(void *address) {
  size_t page_size = subhook_page_size();
  return (void *)((uintptr_t)address & ~(page_size - 1));
}

static int subhook_unprotect(void *address, size_t size) {
  void *page = subhook_page_align(address);
  size_t page_size = subhook_page_size();
  size_t diff = (uintptr_t)address - (uintptr_t)page;
  return mprotect(page, size + diff, PROT_READ | PROT_WRITE | PROT_EXEC);
}

static int subhook_protect(void *address, size_t size) {
  void *page = subhook_page_align(address);
  size_t page_size = subhook_page_size();
  size_t diff = (uintptr_t)address - (uintptr_t)page;
  return mprotect(page, size + diff, PROT_READ | PROT_EXEC);
}
#endif

#if defined(SUBHOOK_X86)
static void *subhook_alloc_trampoline(void *src) {
#if defined(SUBHOOK_X86_64)
  // En x86_64 intentamos asignar cerca de la dirección original si es posible
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  // Simplificación genérica para asignación de memoria ejecutable
#endif
  void *p = malloc(32);
  if (p == NULL)
    return NULL;
  if (subhook_unprotect(p, 32) < 0) {
    free(p);
    return NULL;
  }
  return p;
}
#elif defined(SUBHOOK_ARM64)
static void *subhook_alloc_trampoline(void *src) {
  void *p = malloc(64);
  if (p == NULL)
    return NULL;
  if (subhook_unprotect(p, 64) < 0) {
    free(p);
    return NULL;
  }
  return p;
}
#else
static void *subhook_alloc_trampoline(void *src) {
  void *p = malloc(32);
  if (p == NULL)
    return NULL;
  if (subhook_unprotect(p, 32) < 0) {
    free(p);
    return NULL;
  }
  return p;
}
#endif

SUBHOOK_API subhook_t SUBHOOK_CC subhook_new(void *src, void *dst, int flags) {
  subhook_t hook = (subhook_t)malloc(sizeof(struct subhook));
  if (hook == NULL)
    return NULL;
  hook->src = src;
  hook->dst = dst;
  hook->flags = flags;
  hook->installed = 0;
  hook->trampoline = NULL;
  hook->patch_size = 0;
  return hook;
}

SUBHOOK_API void SUBHOOK_CC subhook_free(subhook_t hook) {
  if (hook->installed)
    subhook_remove(hook);
  if (hook->trampoline != NULL)
    free(hook->trampoline);
  free(hook);
}

SUBHOOK_API void * SUBHOOK_CC subhook_get_src(subhook_t hook) {
  return hook->src;
}

SUBHOOK_API void * SUBHOOK_CC subhook_get_dst(subhook_t hook) {
  return hook->dst;
}

SUBHOOK_API void * SUBHOOK_CC subhook_get_trampoline(subhook_t hook) {
  return hook->trampoline;
}

#if defined(SUBHOOK_X86_32)
SUBHOOK_API int SUBHOOK_CC subhook_install(subhook_t hook) {
  if (hook->installed)
    return -1;
  
  void *src = hook->src;
  void *dst = hook->dst;
  
  if (subhook_unprotect(src, 5) < 0)
    return -1;
    
  hook->patch_size = 5;
  memcpy(hook->original_code, src, 5);
  
  if (hook->trampoline == NULL) {
    hook->trampoline = subhook_alloc_trampoline(src);
    if (hook->trampoline == NULL)
      return -1;
    // Copiar código original al trampolín y agregar salto de retorno
    memcpy(hook->trampoline, hook->original_code, 5);
    unsigned char *tramp_jmp = (unsigned char *)hook->trampoline + 5;
    *tramp_jmp = 0xE9; // JMP rel32
    *(int32_t *)(tramp_jmp + 1) = (uintptr_t)src + 5 - ((uintptr_t)tramp_jmp + 5);
  }
  
  unsigned char *p = (unsigned char *)src;
  *p = 0xE9; // JMP rel32
  *(int32_t *)(p + 1) = (uintptr_t)dst - ((uintptr_t)src + 5);
  
  subhook_protect(src, 5);
  hook->installed = 1;
  return 0;
}
#elif defined(SUBHOOK_ARM64)
SUBHOOK_API int SUBHOOK_CC subhook_install(subhook_t hook) {
  if (hook->installed)
    return -1;

  void *src = hook->src;
  void *dst = hook->dst;

  if (subhook_unprotect(src, 16) < 0)
    return -1;

  hook->patch_size = 16;
  memcpy(hook->original_code, src, 16);

  if (hook->trampoline == NULL) {
    hook->trampoline = subhook_alloc_trampoline(src);
    if (hook->trampoline == NULL)
      return -1;
    memcpy(hook->trampoline, hook->original_code, 16);
  }

  // En ARM64 se escribe la instrucción de salto absoluto (ej. LDR + BR)
  uint32_t *p = (uint32_t *)src;
  // LDR x16, #8 ; BR x16 ; .quad dst
  p[0] = 0x58000050; // ldr x16, [pc, #8]
  p[1] = 0xd61f0200; // br x16
  *(uint64_t *)&p[2] = (uint64_t)dst;

  subhook_protect(src, 16);
  hook->installed = 1;
  return 0;
}
#else
// Implementación por defecto para arquitecturas genéricas
SUBHOOK_API int SUBHOOK_CC subhook_install(subhook_t hook) {
  if (hook->installed)
    return -1;
  void *src = hook->src;
  if (subhook_unprotect(src, 4) < 0)
    return -1;
  hook->patch_size = 4;
  memcpy(hook->original_code, src, 4);
  subhook_protect(src, 4);
  hook->installed = 1;
  return 0;
}
#endif

SUBHOOK_API int SUBHOOK_CC subhook_remove(subhook_t hook) {
  if (!hook->installed)
    return -1;
  void *src = hook->src;
  if (subhook_unprotect(src, hook->patch_size) < 0)
    return -1;
  memcpy(src, hook->original_code, hook->patch_size);
  subhook_protect(src, hook->patch_size);
  hook->installed = 0;
  return 0;
}

SUBHOOK_API subhook_t SUBHOOK_CC subhook_read_coordinator(void *src) {
  return NULL;
}

SUBHOOK_API int SUBHOOK_CC subhook_install_coordinator(subhook_t hook) {
  return -1;
}

SUBHOOK_API int SUBHOOK_CC subhook_remove_coordinator(subhook_t hook) {
  return -1;
}
