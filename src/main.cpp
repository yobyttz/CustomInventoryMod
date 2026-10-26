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
// PUNTEROS PARA addCreativeItemEntry
// ==========================================

uintptr_t base_module_addr = 0;

// Definimos el tipo de función para addCreativeItemEntry basada en tu escaneo previo
typedef void* (*addCreativeItemEntry_t)(void* _this, void* itemEntry);
addCreativeItemEntry_t original_addCreativeItemEntry = nullptr;

subhook_t addCreativeItemEntry_hook;

// Hook pasivo y seguro en addCreativeItemEntry
void* hk_addCreativeItemEntry(void* _this, void* itemEntry) {
    // Aquí interceptamos cuando el juego añade elementos al creativo
    LOGI("[*] addCreativeItemEntry llamado. Elemento creativo detectado.");
    
    // Llamamos limpiamente a la función original para que no se altere ningún ítem del juego
    return original_addCreativeItemEntry(_this, itemEntry);
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

void init_item_injection() {
    FILE* f = fopen(LOG_FILE_PATH, "w");
    if (f != NULL) {
        fprintf(f, "=== INICIO: Mod de Inyección Creativa ===\n");
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

    // Firma exacta de addCreativeItemEntry que ya comprobamos que funciona
    const char* addCreativeItemEntry_sig = "FD 7B BE A9 F4 4F 01 A9 FD 03 00 91 ?? ?? ?? ?? F4 03 00 AA 20 A0 00 91 F3 03 01 AA ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 81 62 40 39 3F 00 20 6B ?? ?? ?? ?? 68 1A 40 F9 00 01 40 F9 ?? ?? ?? ??";
    
    uintptr_t addr = find_signature(base_module_addr, mod_size, addCreativeItemEntry_sig);
    if (addr != 0) {
        LOGI("[+] addCreativeItemEntry encontrado en offset real: 0x%lx", addr - base_module_addr);
        
        addCreativeItemEntry_hook = subhook_new((void*)addr, (void*)hk_addCreativeItemEntry, 0);
        if (addCreativeItemEntry_hook != nullptr) {
            original_addCreativeItemEntry = (addCreativeItemEntry_t)subhook_get_trampoline(addCreativeItemEntry_hook);
            if (subhook_install(addCreativeItemEntry_hook) == 0) {
                LOGI("[+] ¡Hook instalado exitosamente en addCreativeItemEntry!");
            } else {
                LOGE("[-] Error al instalar el hook con subhook_install");
            }
        }
    } else {
        LOGI("[-] No se pudo ubicar addCreativeItemEntry para el hook.");
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando módulo de inyección...");
    init_item_injection();
    return JNI_VERSION_1_6;
}
