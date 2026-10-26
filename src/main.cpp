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
// MOTOR DE ESCANEO DE PATRONES
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

void scan_mod_signatures() {
    FILE* f = fopen(LOG_FILE_PATH, "w");
    if (f != NULL) {
        fprintf(f, "=== INICIO: Escaneo de Firmas de Item Groups ===\n");
        fclose(f);
    }

    uintptr_t base_addr = 0;
    size_t mod_size = 0;
    
    while (base_addr == 0 || mod_size == 0) {
        base_addr = get_module_info("libminecraftpe.so", mod_size);
        if (base_addr == 0) {
            usleep(100000);
        }
    }

    LOGI("[*] Base: 0x%lx | Tamaño ejecutable: %zu bytes", base_addr, mod_size);

    // Lista de firmas recopiladas del mod de tabs/items
    struct SigTarget {
        const char* name;
        const char* sig;
    };

    SigTarget targets[] = {
        {"addLooseCreativeItemsTarget", "FF 03 05 D1 FD 7B 0F A9 FC 67 10 A9 F8 5F 11 A9 F6 57 12 A9 F4 4F 13 A9 FD C3 03 91 55 D0 3B D5 B4 63 01 D1 F3 03 00 AA A8 16 40 F9 A8 83 1F F8 A0 83 1A F8 80 22 00 91 ?? ?? ?? ?? E8 23 00 91"},
        {"lockItemRegistry",            "FD 7B BE A9 F4 4F 01 A9 FD 03 00 91 F4 03 00 AA 00 04 40 F9 F3 03 08 AA 1F 7D 00 A9 40 01 00 B4 ?? ?? ?? ?? 60 06 00 F9 E0 00 00 B4 88 02 40 F9 68 02 00 F9 88 00 00 B4"},
        {"getCreativeCategory",         "09 28 40 F9 89 06 00 B4 20 01 67 9E 28 1C 40 92 00 58 20 0E 00 38 30 2E 0C 00 26 1E 9F 05 00 F1 ?? ?? ?? ?? 2A 05 00 51 4A 01 08 8A ?? ?? ?? ?? 3F 01 08 EB EA 03 08 AA ?? ?? ?? ?? 2A 1D 00 12"},
        {"addChildGroup",               "FF 03 02 D1 FD 7B 02 A9 FC 6F 03 A9 FA 67 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9 FD 83 00 91 59 D0 3B D5 28 17 40 F9 A8 83 1F F8 08 1C 40 F9 ?? ?? ?? ?? F4 03 00 AA 00 00 01 91 F5 03 02 AA"},
        {"newItemEntry",                "FF 83 01 D1 FD 7B 02 A9 F8 5F 03 A9 F6 57 04 A9 F4 4F 05 A9 FD 83 00 91 ?? ?? ?? ?? F3 03 00 AA 6A 74 91 52 E8 16 40 F9 CA 45 B7 72 F5 03 02 AA F6 03 01 AA A8 83 1F F8 14 24 42 A9 E0 07 00 F9"},
        {"addCreativeItemEntry",        "FD 7B BE A9 F4 4F 01 A9 FD 03 00 91 ?? ?? ?? ?? F4 03 00 AA 20 A0 00 91 F3 03 01 AA ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 81 62 40 39 3F 00 20 6B ?? ?? ?? ?? 68 1A 40 F9 00 01 40 F9 ?? ?? ?? ??"},
        {"updateNetIdMap",              "FF C3 01 D1 FD 7B 02 A9 F9 1B 00 F9 F8 5F 04 A9 F6 57 05 A9 F4 4F 06 A9 FD 83 00 91 56 D0 3B D5 F3 03 00 AA C8 16 40 F9 A8 83 1F F8 08 44 40 F9 ?? ?? ?? ?? 74 42 40 F9 ?? ?? ?? ?? E0 03 14 AA"},
        {"itemInstanceNameCtor",        "FD 7B BE A9 F3 0B 00 F9 FD 03 00 91 F3 03 00 AA ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 68 02 00 F9 F3 0B 40 F9 FD 7B C2 A8 C0 03 5F D6 FF 43 03 D1 FD 7B 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9"},
        {"itemInstanceDtor",            "FD 7B BD A9 F5 0B 00 F9 F4 4F 02 A9 FD 03 00 91 F3 03 00 AA 00 3C 40 F9 ?? ?? ?? ?? ?? ?? ?? ?? 7F 3E 00 F9 68 02 00 F9 ?? ?? ?? ?? 08 00 40 F9 08 05 40 F9 00 01 3F D6 60 2A 40 F9 ?? ?? ?? ??"},
        {"itemGetItem",                 "08 04 40 F9 68 00 00 B4 00 01 40 F9 C0 03 5F D6 E0 03 1F AA C0 03 5F D6 ?? ?? ?? ?? F5 0B 00 F9"},
        {"itemGetFullItemName",         "00 A0 03 91 ?? ?? ?? 14 00 A0 03 91 C0 03 5F D6 ?? ?? ?? ?? FD 7B 03 A9"}
    };

    for (const auto& target : targets) {
        uintptr_t addr = find_signature(base_addr, mod_size, target.sig);
        if (addr != 0) {
            LOGI("[+] Encontrado [%s] -> Offset: 0x%lx", target.name, addr - base_addr);
        } else {
            LOGI("[-] No encontrado: [%s]", target.name);
        }
    }
    
    LOGI("[*] Escaneo de firmas finalizado.");
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Iniciando escáner de firmas...");
    scan_mod_signatures();
    return JNI_VERSION_1_6;
}
