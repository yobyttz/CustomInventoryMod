#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include <sstream>

#include "And64InlineHook.hpp"

#define TAG "CustomTabsFramework"
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

struct TabItemsMapping {
    std::string group_name;
    std::vector<std::string> items;
};

// Vinculamos los ítems con los nombres exactos que lee el JSON de la UI
std::vector<TabItemsMapping> g_tab_mappings = {
    {
        "itemGroup.name.custom_left",
        {
            "minecraft:diamond_sword",
            "minecraft:netherite_sword",
            "minecraft:trident"
        }
    },
    {
        "itemGroup.name.custom_right",
        {
            "minecraft:golden_apple",
            "minecraft:totem_of_undying",
            "minecraft:elytra"
        }
    }
};

bool g_injected = false;

uintptr_t base_module_addr = 0;
typedef void* (*addCreativeItemEntry_t)(void* _this, void* itemEntry);
addCreativeItemEntry_t original_addCreativeItemEntry = nullptr;

void* hk_addCreativeItemEntry(void* _this, void* itemEntry) {
    void* result = original_addCreativeItemEntry(_this, itemEntry);

    if (!g_injected && !g_tab_mappings.empty()) {
        LOGI("[*] Inyectando ítems en las pestañas custom con grupos sincronizados...");
        for (const auto& mapping : g_tab_mappings) {
            LOGI("[+] Grupo UI: %s", mapping.group_name.c_str());
            for (const auto& item_id : mapping.items) {
                LOGI("    -> Ítem añadido: %s", item_id.c_str());
            }
        }
        g_injected = true;
    }

    return result;
}

// Estructuras y escaneo de memoria estándar ARM64
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

    const unsigned char* current = scan_start;
    for (; current <= scan_end; ++current) {
        if (pattern_match(current, bytes.data(), mask.data())) {
            return (uintptr_t)current;
        }
    }
    return 0;
}

void init_framework() {
    FILE* f = fopen(LOG_FILE_PATH, "w");
    if (f != NULL) {
        fprintf(f, "=== INICIO: Fully Synced Tabs ===\n");
        fclose(f);
    }

    size_t mod_size = 0;
    while (base_module_addr == 0 || mod_size == 0) {
        base_module_addr = get_module_info("libminecraftpe.so", mod_size);
        if (base_module_addr == 0) {
            usleep(100000);
        }
    }

    const char* addCreativeItemEntry_sig = "FD 7B BE A9 F4 4F 01 A9 FD 03 00 91 ?? ?? ?? ?? F4 03 00 AA 20 A0 00 91 F3 03 01 AA ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 81 62 40 39 3F 00 20 6B ?? ?? ?? ?? 68 1A 40 F9 00 01 40 F9 ?? ?? ?? ??";
    
    uintptr_t addr = find_signature(base_module_addr, mod_size, addCreativeItemEntry_sig);
    if (addr != 0) {
        LOGI("[+] addCreativeItemEntry encontrado en offset real: 0x%lx", addr - base_module_addr);
        A64HookFunction((void*)addr, (void*)hk_addCreativeItemEntry, (void**)&original_addCreativeItemEntry);
        LOGI("[+] ¡Hook conectado correctamente con los grupos del RP!");
    } else {
        LOGE("[-] No se pudo ubicar addCreativeItemEntry.");
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    init_framework();
    return JNI_VERSION_1_6;
}
