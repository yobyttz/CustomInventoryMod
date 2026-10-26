#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <sstream>
#include "subhook.h"

#define TAG "backpack"
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
// PUNTEROS Y HOOKS PARA CREATIVE TABS
// ==========================================

uintptr_t base_module_addr = 0;

// Definimos los tipos de las funciones clave basadas en los offsets encontrados
// (Usamos void* temporalmente para asegurar compatibilidad de llamadas)
typedef void* (*addChildGroup_t)(void* self, void* group1, void* group2);
addChildGroup_t original_addChildGroup = nullptr;

subhook_t addChildGroup_hook;

// Nuestra función interceptada (Hook) para cuando el juego añada grupos de ítems
void* hk_addChildGroup(void* self, void* group1, void* group2) {
    // Aquí es donde interceptaremos la llamada para inyectar nuestras 2 pestañas custom
    LOGI("[*] Interceptado: addChildGroup llamado. ¡Inyectando pestañas personalizadas!");
    
    // Llamamos a la función original para no romper la cadena de grupos nativos del juego
    return original_addChildGroup(self, group1, group2);
}

// ==========================================
// MOTOR DE ESCANEO
// ==========================================

struct dl_iterate_context {
    const char* name;
    uintptr_t base_addr;
    size_t mem_size;
};

static int dl_iterate_callback(struct dl_phdr_info *info, size_t size, void *data) {
    auto* ctx = reinterpret_cast<dl_iterate_context*>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, ctx->name)) {
        ctx->base_addr = info->dlpi_addr;
        for (int i = 0; i < info->dlpi_phnum; i++) {
            if (info->dlpi_phdr[i].p_type == PT_LOAD && (info->dlpi_phdr[i].p_flags & PF_X)) {
                ctx->mem_size = info->dlpi_phdr[i].p_memsz;
                break;
            }
        }
        return 1;
    }
    return 0;
}

uintptr_t get_module_info(const char* module_name, size_t &out_size) {
    dl_iterate_context ctx = { module_name, 0, 0 };
    dl_iterate_phdr(dl_iterate_callback, &ctx);
    out_size = ctx.mem_size;
    return ctx.base_addr;
}

bool pattern_match(const unsigned char* addr, const unsigned char* pattern, const char* mask) {
    for (; *mask; ++mask, ++addr, ++pattern) {
        if (*mask == 'x' && *addr != *pattern) {
            return false;
        }
    }
    return (*mask) == 0;
}

uintptr_t find_signature(uintptr_t base, size_t size, const char* sig) {
    std::vector<unsigned char> bytes;
    std::vector<char> mask;
    
    std::stringstream ss(sig);
    std::string byte_str;
    while (ss >> byte_str) {
        if (byte_str == "??" || byte_str == "?") {
            bytes.push_back(0);
            mask.push_back('?');
        } else {
            bytes.push_back((unsigned char)strtoul(byte_str.c_str(), nullptr, 16));
            mask.push_back('x');
        }
    }
    
    size_t pattern_length = bytes.size();
    if (pattern_length == 0 || size < pattern_length) return 0;

    const unsigned char* scan_start = (const unsigned char*)base;
    const unsigned char* scan_end = scan_start + size - pattern_length;

    for (const unsigned char* current = scan_start; current <= scan_end; ++current) {
        if (pattern_match(current, bytes.data(), mask.data())) {
            return (uintptr_t)current;
        }
    }
    return 0;
}

void init_custom_tabs() {
    FILE* f = fopen(LOG_FILE_PATH, "w");
    if (f != NULL) {
        fprintf(f, "=== INICIO: Mod de Pestañas Custom ===\n");
        fclose(f);
    }

    size_t mod_size = 0;
    while (base_module_addr == 0 || mod_size == 0) {
        base_module_addr = get_module_info("libminecraftpe.so", mod_size);
        if (base_module_addr == 0) {
            usleep(100000);
        }
    }

    LOGI("[*] Base: 0x%lx | Tamaño: %zu bytes", base_module_addr, mod_size);

    // Firma de addChildGroup que nos dio éxito en el escaneo anterior
    const char* addChildGroup_sig = "FF 03 02 D1 FD 7B 02 A9 FC 6F 03 A9 FA 67 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9 FD 83 00 91 59 D0 3B D5 28 17 40 F9 A8 83 1F F8 08 1C 40 F9 ?? ?? ?? ?? F4 03 00 AA 00 00 01 91 F5 03 02 AA";
    
    uintptr_t addr = find_signature(base_module_addr, mod_size, addChildGroup_sig);
    if (addr != 0) {
        LOGI("[+] addChildGroup encontrado en offset real: 0x%lx", addr - base_module_addr);
        
        // CORREGIDO: Añadido el tercer parámetro '0' para cumplir con la firma de subhook
        addChildGroup_hook = subhook_new((void*)addr, (void*)hk_addChildGroup, 0);
        if (addChildGroup_hook != nullptr) {
            original_addChildGroup = (addChildGroup_t)subhook_get_trampoline(addChildGroup_hook);
            if (subhook_install(addChildGroup_hook) == 0) {
                LOGI("[+] ¡Hook instalado exitosamente en addChildGroup!");
            } else {
                LOGE("[-] Error al instalar el hook con subhook_install");
            }
        }
    } else {
        LOGI("[-] No se pudo ubicar addChildGroup para el hook.");
    }
}


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando módulo de pestañas...");
    init_custom_tabs();
    return JNI_VERSION_1_6;
}
