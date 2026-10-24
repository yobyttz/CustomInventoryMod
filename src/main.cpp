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

typedef void* (*ContainerControllerInit_t)(void* self);
ContainerControllerInit_t original_controller_init = nullptr;

void* hooked_container_controller_init(void* self) {
    LOGI("[+] ¡Se interceptó el controlador de contenedores / inventario!");
    return original_controller_init(self);
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
        fprintf(f, "=== INICIO DEL MOD: CustomInventoryMod ===\n");
        fclose(f);
    }

    uintptr_t base_addr = 0;
    while (base_addr == 0) {
        base_addr = get_module_base("libminecraftpe.so");
        if (base_addr == 0) {
            usleep(100000); // Esperar 100ms si el juego aún está cargando la librería
        }
    }
    
    LOGI("[*] libminecraftpe.so encontrado en la base: 0x%lx", base_addr);

    // Nota: Usamos una dirección segura temporal para evitar crashes hasta definir el offset exacto
    uintptr_t target_offset = 0x0; // Sin hook activo de prueba por ahora para garantizar estabilidad
    
    if (target_offset != 0) {
        void* target_func = reinterpret_cast<void*>(base_addr + target_offset);
        subhook_t hook = subhook_new(target_func, (void*)hooked_container_controller_init, SUBHOOK_64BIT_OFFSET);
        if (subhook_install(hook) < 0) {
            LOGE("[-] ¡Error al instalar el hook con Subhook!");
        } else {
            LOGI("[+] ¡Hook instalado exitosamente!");
            original_controller_init = (ContainerControllerInit_t)subhook_get_trampoline(hook);
        }
    } else {
        LOGI("[*] Mod ejecutándose en modo estable (esperando offset definitivo).");
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando mod nativo en GitHub Actions / LeviLauncher...");
    
    // Lanzamos la inicialización en un hilo independiente para no congelar el arranque de Minecraft
    // (O mantenemos la llamada directa si el juego maneja bien el tiempo de espera)
    init_hooks();

    return JNI_VERSION_1_6;
}
