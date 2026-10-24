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
    ElfW(Addr) map_start;
    ElfW(Addr) map_end;
};

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data) {
    auto* ctx = reinterpret_cast<dl_iterate_context*>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, ctx->name)) {
        ctx->base_addr = info->dlpi_addr;
        
        for (int i = 0; i < info->dlpi_phnum; i++) {
            const auto* phdr = &info->dlpi_phdr[i];
            // Buscamos tanto secciones ejecutables (.text) como de datos de solo lectura (.rodata)
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

// 1. Buscar la cadena exacta en la memoria
uintptr_t find_string_in_memory(uintptr_t start, uintptr_t end, const char* str) {
    size_t len = strlen(str);
    for (uintptr_t addr = start; addr < end - len; addr++) {
        if (memcmp(reinterpret_cast<void*>(addr), str, len) == 0) {
            return addr;
        }
    }
    return 0;
}

// 2. Escanear referencias (punteros de 8 bytes alineados) que apunten a una dirección objetivo
void scan_and_log_references(uintptr_t start, uintptr_t end, uintptr_t target_ptr, uintptr_t base_addr) {
    LOGI("[*] Buscando referencias en memoria a la dirección: 0x%lx", target_ptr);
    int found_count = 0;

    // En arquitecturas de 64 bits (ARM64), los punteros están alineados a 8 bytes
    for (uintptr_t addr = start; addr < end - 8; addr += 8) {
        uintptr_t* ptr_val = reinterpret_cast<uintptr_t*>(addr);
        if (*ptr_val == target_ptr) {
            found_count++;
            uintptr_t offset = addr - base_addr;
            LOGI("[+] ¡Referencia #%d! RAM: 0x%lx | Offset Estático: 0x%lx", found_count, addr, offset);
            
            if (found_count >= 15) { // Mostrar un límite razonable para no saturar el log
                LOGI("[*] Se alcanzó el límite de referencias mostradas.");
                break;
            }
        }
    }

    if (found_count == 0) {
        LOGI("[-] No se encontraron referencias de punteros directos a esta cadena.");
    }
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

    // Buscar la cadena del controlador de inventario
    const char* search_term = "ContainerScreenController";
    uintptr_t string_addr = find_string_in_memory(ctx.map_start, ctx.map_end, search_term);

    if (string_addr != 0) {
        LOGI("[+] Cadena '%s' encontrada en RAM: 0x%lx (Offset: 0x%lx)", 
             search_term, string_addr, string_addr - ctx.base_addr);
        
        // Ejecutar el escáner para ver quién apunta a esta cadena en el binario
        scan_and_log_references(ctx.map_start, ctx.map_end, string_addr, ctx.base_addr);
    } else {
        LOGI("[-] No se encontró la cadena especificada.");
    }

    // Offset provisional actual (lo cambiaremos cuando veas el resultado del escáner)
    uintptr_t target_offset = 0x123456; 
    void* target_func = reinterpret_cast<void*>(ctx.base_addr + target_offset);

    subhook_t hook = subhook_new(target_func, (void*)hooked_container_controller_init, SUBHOOK_64BIT_OFFSET);
    if (subhook_install(hook) < 0) {
        LOGE("[-] ¡Error al instalar el hook con Subhook!");
    } else {
        LOGI("[+] ¡Hook instalado exitosamente!");
        original_controller_init = (ContainerControllerInit_t)subhook_get_trampoline(hook);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando mod nativo en LeviLauncher...");
    init_hooks();
    return JNI_VERSION_1_6;
}
