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
    LOGI("[+] ¡ÉXITO! Se interceptó el controlador de inventario.");
    if (original_controller_init) {
        return original_controller_init(self);
    }
    return nullptr;
}

struct dl_iterate_context {
    const char* name;
    uintptr_t base_addr;
    ElfW(Addr) map_start;
    ElfW(Addr) map_end;
};

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data) {
    auto* ctx = reinterpret_cast<dl_iterate_context*>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, ctx->name)) {
        ctx->base_addr = info->dlpi_addr;
        
        for (int i = 0; i < info->dlpi_phnum; i++) {
            const auto* phdr = &info->dlpi_phdr[i];
            if (phdr->p_type == PT_LOAD) {
                if (ctx->map_start == 0) {
                    ctx->map_start = info->dlpi_addr + phdr->p_vaddr;
                }
                ctx->map_end = info->dlpi_addr + phdr->p_vaddr + phdr->p_memsz;
            }
        }
        return 1;
    }
    return 0;
}

void init_hooks() {
    FILE* f = fopen(LOG_FILE_PATH, "w");
    if (f != NULL) {
        fprintf(f, "=== INICIO DEL MOD: CustomInventoryMod ===\n");
        fclose(f);
    }

    dl_iterate_context ctx = { "libminecraftpe.so", 0, 0, 0 };
    while (ctx.base_addr == 0) {
        dl_iterate_phdr(dl_iterate_callback, &ctx);
        if (ctx.base_addr == 0) {
            usleep(100000);
        }
    }
    
    LOGI("[*] libminecraftpe.so base: 0x%lx", ctx.base_addr);

    // 1. Localizar la cadena "ContainerScreenController" con su offset exacto ya conocido
    uintptr_t string_offset = 0x29c7706;
    uintptr_t string_addr = ctx.base_addr + string_offset;
    LOGI("[*] Usando dirección de cadena: 0x%lx", string_addr);

    // 2. Búsqueda acotada y segura de referencias a la cadena en la sección de datos (.rodata / .data)
    // Esto encuentra el puntero del typeinfo sin riesgo de crash
    uintptr_t typeinfo_name_ptr = 0;
    for (uintptr_t addr = ctx.map_start; addr < ctx.map_end - 8; addr += 8) {
        // Validar que la lectura sea segura comprobando los límites
        uintptr_t* val = reinterpret_cast<uintptr_t*>(addr);
        if (*val == string_addr) {
            typeinfo_name_ptr = addr;
            LOGI("[+] Puntero al nombre encontrado en RAM: 0x%lx (Offset: 0x%lx)", addr, addr - ctx.base_addr);
            break; // Tomamos la primera coincidencia válida del descriptor
        }
    }

    if (typeinfo_name_ptr != 0) {
        LOGI("[*] Descriptor de tipo localizado con éxito.");
        // A partir de aquí podemos ubicar la vtable en siguientes iteraciones
    } else {
        LOGI("[-] No se pudo aislar el puntero al nombre de forma automática.");
    }

    // Modo estable por ahora para verificar el log en Download
    LOGI("[*] Mod ejecutándose de forma estable. Preparando vtable...");
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando mod nativo en LeviLauncher...");
    init_hooks();
    return JNI_VERSION_1_6;
}
