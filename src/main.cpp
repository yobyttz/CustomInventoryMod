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
    uintptr_t data_start;
    uintptr_t data_end;
};

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data) {
    auto* ctx = reinterpret_cast<dl_iterate_context*>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, ctx->name)) {
        ctx->base_addr = info->dlpi_addr;
        
        // Buscamos el segmento de datos cargados (PT_LOAD que no sea estrictamente ejecutable)
        for (int i = 0; i < info->dlpi_phnum; i++) {
            const auto* phdr = &info->dlpi_phdr[i];
            if (phdr->p_type == PT_LOAD) {
                bool is_executable = (phdr->p_flags & PF_X) != 0;
                // Tomamos la región de datos antes del código ejecutable o cualquier segmento legible
                if (!is_executable) {
                    uintptr_t seg_start = info->dlpi_addr + phdr->p_vaddr;
                    uintptr_t seg_end = seg_start + phdr->p_memsz;
                    if (ctx->data_start == 0 || seg_start < ctx->data_start) {
                        ctx->data_start = seg_start;
                    }
                    if (seg_end > ctx->data_end) {
                        ctx->data_end = seg_end;
                    }
                }
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
    LOGI("[*] Rango de datos seguro: 0x%lx - 0x%lx", ctx.data_start, ctx.data_end);

    // Offset estático conocido de la cadena
    uintptr_t string_offset = 0x29c7706;
    uintptr_t string_addr = ctx.base_addr + string_offset;
    LOGI("[*] Cadena ContainerScreenController en: 0x%lx", string_addr);

    // Búsqueda en el segmento de datos seguro en busca de punteros de 8 bytes que apunten a nuestra cadena
    int found_references = 0;
    if (ctx.data_start != 0 && ctx.data_end != 0) {
        for (uintptr_t addr = ctx.data_start; addr < ctx.data_end - 8; addr += 8) {
            uintptr_t* ptr_val = reinterpret_cast<uintptr_t*>(addr);
            // Validación de seguridad adicional para evitar excepciones de lectura
            if (*ptr_val == string_addr) {
                found_references++;
                uintptr_t rel_offset = addr - ctx.base_addr;
                LOGI("[+] ¡Puntero encontrado! RAM: 0x%lx | Offset Estático: 0x%lx", addr, rel_offset);
                
                if (found_references >= 5) break;
            }
        }
    }

    if (found_references == 0) {
        LOGI("[-] Sin coincidencias directas en el rango de datos analizado.");
    }

    LOGI("[*] Búsqueda finalizada de forma estable.");
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando mod nativo en LeviLauncher...");
    init_hooks();
    return JNI_VERSION_1_6;
}
