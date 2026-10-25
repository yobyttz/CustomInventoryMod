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

// Firma de un solo argumento (puntero `this` de la instancia del controlador de inventario)
typedef void* (*InventoryFunc_t)(void* self);
InventoryFunc_t original_inventory_func = nullptr;

void* hooked_inventory_func(void* self) {
    // Cada vez que entre aquí al abrir el inventario, lo escribirá al instante
    LOGI("[+] ¡INVENTARIO CAPTURADO! Instancia (self): %p", self);
    
    if (original_inventory_func) {
        return original_inventory_func(self);
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
        fprintf(f, "=== INICIO DEL MOD: Caza de Inventario ===\n");
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

    // El offset exacto que demostró reaccionar con el inventario
    uintptr_t target_function_offset = 0x29c5000; 

    void* target_func = reinterpret_cast<void*>(base_addr + target_function_offset);
    
    LOGI("[*] Instalando hook de inventario en: 0x%lx", (uintptr_t)target_func);
    
    subhook_t hook = subhook_new(target_func, (void*)hooked_inventory_func, SUBHOOK_64BIT_OFFSET);
    if (hook == nullptr) {
        LOGE("[-] Error al crear el subhook.");
    } else {
        int install_result = subhook_install(hook);
        if (install_result < 0) {
            LOGE("[-] Error al instalar hook: %d", install_result);
        } else {
            LOGI("[+] ¡Instalado con éxito en 0x%lx!", (uintptr_t)target_func);
            original_inventory_func = (InventoryFunc_t)subhook_get_trampoline(hook);
        }
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando mod de inventario...");
    init_hooks();
    return JNI_VERSION_1_6;
}
