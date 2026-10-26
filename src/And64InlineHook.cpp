#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include "And64InlineHook.hpp"

#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define PAGE_START(addr) ((addr) & ~(PAGE_SIZE - 1))

static int mprotect_safe(void *addr, size_t len, int prot) {
    uintptr_t start = (uintptr_t)PAGE_START((uintptr_t)addr);
    uintptr_t end = (uintptr_t)PAGE_START((uintptr_t)addr + len);
    size_t page_len = (end - start) + PAGE_SIZE;
    return mprotect((void *)start, page_len, prot);
}

bool A64HookFunction(void *symbol, void *replace, void **result) {
    if (!symbol || !replace) return false;
    
    uint32_t *a_symbol = (uint32_t *)symbol;
    
    // Reservamos memoria para el trampolín
    void *trampoline = malloc(32);
    if (!trampoline) return false;
    
    if (mprotect_safe(trampoline, 32, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        free(trampoline);
        return false;
    }
    
    if (mprotect_safe(symbol, 32, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        free(trampoline);
        return false;
    }
    
    // Copiamos las primeras instrucciones originales al trampolín (2 instrucciones = 8 bytes)
    uint32_t *a_trampoline = (uint32_t *)trampoline;
    a_trampoline[0] = a_symbol[0];
    a_trampoline[1] = a_symbol[1];
    
    // Escribimos el salto de vuelta desde el trampolín al resto de la función original
    uint64_t next_addr = (uint64_t)(symbol) + 8;
    a_trampoline[2] = 0x58000050; // ldr x16, #8
    a_trampoline[3] = 0xd61f0200; // br x16
    *(uint64_t *)((uintptr_t)trampoline + 16) = next_addr;
    
    // Escribimos el salto desde la función original hacia nuestra función reemplazo (replace)
    a_symbol[0] = 0x58000050; // ldr x16, #8
    a_symbol[1] = 0xd61f0200; // br x16
    *(uint64_t *)((uintptr_t)symbol + 8) = (uint64_t)replace;
    
    // CORREGIDO: Casteo explícito a (char*) para satisfacer los tipos de la NDK
    __builtin___clear_cache((char *)trampoline, (char *)trampoline + 32);
    __builtin___clear_cache((char *)symbol, (char *)symbol + 32);
    
    if (result) {
        *result = trampoline;
    }
    return true;
}
