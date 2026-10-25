#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "subhook.h"

#define TAG "CustomInventoryMod"
const char* LOG_FILE_PATH = "/storage/emulated/0/Download/mod_debug.txt";

void write_log(const char* fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, TAG, fmt, args1);
    va_end(args1);

    FILE* f = fopen(LOG_FILE_PATH, "a");
    if (f != NULL) {
        va_list args2;
        va_start(args2, fmt);
        vfprintf(f, fmt, args2);
        fprintf(f, "\n");
        va_end(args2);
        fclose(f);
    }
}

#define LOGI(...) write_log("[INFO] " __VA_ARGS__)
#define LOGE(...) write_log("[ERROR] " __VA_ARGS__)

// ==========================================
// INTENTO 2: Hook simplificado de 1 argumento
// ==========================================
subhook_t global_hook = nullptr;

void* hooked_simple_passthrough(void* self) {
    LOGI("[+] ¡Intento 2 disparado! self: %p", self);
    
    // Llamada directa al código original mediante el trampoline
    auto original_func = (void* (*)(void*))subhook_get_trampoline(global_hook);
    if (original_func) {
        return original_func(self);
    }
    return nullptr;
}

struct dl_iterate_context {
    const char* name;
    uintptr_t base_addr;
};

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data) {
    auto* ctx = reinterpret_cast<dl_iterate_context*>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, ctx->name)) {
        ctx->base_addr = info->dlpi_addr;
        return 1;
    }
    return 0;
}

uintptr_t get_module_base(const char* module_name) {
    dl_iterate_context ctx = { module_name, 0 };
    dl_iterate_phdr(dl_iterate_callback, &ctx);
    return ctx.base_addr;
}

void init_hooks() {
    FILE* f = fopen(LOG_FILE_PATH, "w");
    if (f != NULL) {
        fprintf(f, "=== INICIO DEL MOD: Intento 2 ===\n");
        fclose(f);
    }

    uintptr_t base_addr = 0;
    while (base_addr == 0) {
        base_addr = get_module_base("libminecraftpe.so");
        if (base_addr == 0) {
            usleep(100000);
        }
    }
    
    LOGI("[*] libminecraftpe.so base: 0x%lx", base_addr);

    uintptr_t target_function_offset = 0x1dc7ac0; 
    void* target_func = reinterpret_cast<void*>(base_addr + target_function_offset);
    
    LOGI("[*] Instalando hook Intento 2 en: 0x%lx", (uintptr_t)target_func);
    
    global_hook = subhook_new(target_func, (void*)hooked_simple_passthrough, SUBHOOK_64BIT_OFFSET);
    if (global_hook == nullptr) {
        LOGE("[-] Error al crear el subhook.");
    } else {
        int install_result = subhook_install(global_hook);
        if (install_result < 0) {
            LOGE("[-] Error al instalar hook: %d", install_result);
        } else {
            LOGI("[+] ¡Instalado con éxito en 0x%lx!", (uintptr_t)target_func);
        }
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando mod (Intento 2)...");
    init_hooks();
    return JNI_VERSION_1_6;
}
