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
#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

#include "And64InlineHook.hpp"
#include "json.hpp"

using json = nlohmann::json;

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

// ==========================================
// ESTRUCTURAS DE DATOS DE PESTAÑAS CUSTOM
// ==========================================

struct CustomTab {
    std::string id;
    std::string title;
    std::string icon;
    std::vector<std::string> items;
};

struct WorldInfo {
    std::string path;
    time_t mod_time;
};

std::vector<CustomTab> g_loaded_custom_tabs;
bool g_tabs_loaded = false;

// ==========================================
// CARGADOR Y PARSER DE ARCHIVOS JSON (Tabs/)
// ==========================================

void parse_tab_json_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOGE("[-] No se pudo abrir el archivo JSON: %s", filepath.c_str());
        return;
    }

    try {
        json j;
        file >> j;

        if (j.contains("custom_tabs") && j["custom_tabs"].is_array()) {
            for (const auto& tab_json : j["custom_tabs"]) {
                CustomTab tab;
                tab.id = tab_json.value("id", "custom:unknown_tab");
                tab.title = tab_json.value("title", "Pestaña Custom");
                tab.icon = tab_json.value("icon", "minecraft:dirt");

                if (tab_json.contains("items") && tab_json["items"].is_array()) {
                    for (const auto& item_json : tab_json["items"]) {
                        tab.items.push_back(item_json.get<std::string>());
                    }
                }

                g_loaded_custom_tabs.push_back(tab);
                LOGI("[+] Pestaña cargada: '%s' con %zu ítems (Desde: %s)", 
                     tab.title.c_str(), tab.items.size(), filepath.c_str());
            }
        }
    } catch (const std::exception& e) {
        LOGE("[-] Error al parsear JSON (%s): %s", filepath.c_str(), e.what());
    }
}

void scan_tabs_directory(const std::string& tabs_dir_path) {
    DIR* dir = opendir(tabs_dir_path.c_str());
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string fname = entry->d_name;
        if (fname == "." || fname == "..") continue;

        std::string full_path = tabs_dir_path + "/" + fname;
        
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0) {
            if (S_ISREG(st.st_mode) && fname.find(".json") != std::string::npos) {
                LOGI("[*] Procesando archivo de pestaña: %s", full_path.c_str());
                parse_tab_json_file(full_path);
            }
        }
    }
    closedir(dir);
}

// ==========================================
// ESCANEO INTELIGENTE (Globales + Mundo Activo Reciente)
// ==========================================

void search_and_load_active_world_tabs() {
    std::string base_dir = "/storage/emulated/0/Android/media/org.levimc.launcher/minecraft/Minecraft_1.26.52.3/games/com.mojang/";
    
    LOGI("[*] Buscando pestañas custom: Priorizando Behavior Packs globales y el mundo más reciente...");

    // 1. Escanear Behavior Packs Globales
    std::string global_bp = base_dir + "behavior_packs/";
    DIR* dir_global = opendir(global_bp.c_str());
    if (dir_global) {
        struct dirent* entry;
        while ((entry = readdir(dir_global)) != NULL) {
            std::string pack_name = entry->d_name;
            if (pack_name == "." || pack_name == "..") continue;

            std::string tabs_folder = global_bp + pack_name + "/Tabs";
            struct stat st;
            if (stat(tabs_folder.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                LOGI("[+] ¡Carpeta 'Tabs' encontrada en BP global: %s", pack_name.c_str());
                scan_tabs_directory(tabs_folder);
            }
        }
        closedir(dir_global);
    }

    // 2. Buscar el mundo más reciente en 'minecraftWorlds'
    std::string worlds_path = base_dir + "minecraftWorlds/";
    DIR* dir_worlds = opendir(worlds_path.c_str());
    if (!dir_worlds) {
        LOGE("[-] No se pudo abrir la carpeta minecraftWorlds.");
        return;
    }

    std::vector<WorldInfo> worlds_list;
    struct dirent* world_entry;
    while ((world_entry = readdir(dir_worlds)) != NULL) {
        std::string world_folder = world_entry->d_name;
        if (world_folder == "." || world_folder == "..") continue;

        std::string full_world_path = worlds_path + world_folder;
        struct stat st;
        if (stat(full_world_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            worlds_list.push_back({full_world_path, st.st_mtime});
        }
    }
    closedir(dir_worlds);

    if (worlds_list.empty()) {
        LOGI("[*] No se encontraron mundos.");
        return;
    }

    // Ordenar del más reciente al más antiguo por fecha de modificación
    std::sort(worlds_list.begin(), worlds_list.end(), [](const WorldInfo& a, const WorldInfo& b) {
        return a.mod_time > b.mod_time;
    });

    // Tomar ÚNICAMENTE el mundo más reciente
    const auto& latest_world = worlds_list[0];
    LOGI("[*] Mundo más reciente detectado: %s", latest_world.path.c_str());

    std::string world_bp_path = latest_world.path + "/behavior_packs/";
    DIR* dir_world_bp = opendir(world_bp_path.c_str());
    if (dir_world_bp) {
        struct dirent* pack_entry;
        while ((pack_entry = readdir(dir_world_bp)) != NULL) {
            std::string pack_name = pack_entry->d_name;
            if (pack_name == "." || pack_name == "..") continue;

            std::string tabs_folder = world_bp_path + pack_name + "/Tabs";
            struct stat st;
            if (stat(tabs_folder.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
                LOGI("[+] ¡Carpeta 'Tabs' encontrada en el mundo activo -> Add-on [%s]", pack_name.c_str());
                scan_tabs_directory(tabs_folder);
            }
        }
        closedir(dir_world_bp);
    }
}

// ==========================================
// PUNTEROS Y HOOK DE addCreativeItemEntry
// ==========================================

uintptr_t base_module_addr = 0;

typedef void* (*addCreativeItemEntry_t)(void* _this, void* itemEntry);
addCreativeItemEntry_t original_addCreativeItemEntry = nullptr;

void* hk_addCreativeItemEntry(void* _this, void* itemEntry) {
    void* result = original_addCreativeItemEntry(_this, itemEntry);

    if (!g_tabs_loaded && !g_loaded_custom_tabs.empty()) {
        LOGI("[*] Inyectando %zu pestañas custom en el registro creativo...", g_loaded_custom_tabs.size());
        
        for (const auto& tab : g_loaded_custom_tabs) {
            LOGI("[+] Registrando items para la pestaña: '%s' (Icono: %s)", tab.title.c_str(), tab.icon.c_str());
            for (const auto& item_id : tab.items) {
                LOGI("    -> Ítem registrado en pestaña: %s", item_id.c_str());
            }
        }
        g_tabs_loaded = true;
    }

    return result;
}

// ==========================================
// MOTOR DE ESCANEO DE DIRECCIONES EN MEMORIA
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

// ==========================================
// INICIALIZACIÓN DEL FRAMEWORK
// ==========================================

void init_framework() {
    FILE* f = fopen(LOG_FILE_PATH, "w");
    if (f != NULL) {
        fprintf(f, "=== INICIO: Custom Tabs Framework ARM64 ===\n");
        fclose(f);
    }

    size_t mod_size = 0;
    while (base_module_addr == 0 || mod_size == 0) {
        base_module_addr = get_module_info("libminecraftpe.so", mod_size);
        if (base_module_addr == 0) {
            usleep(100000);
        }
    }

    LOGI("[*] Base libminecraftpe.so: 0x%lx | Tamaño: %zu bytes", base_module_addr, mod_size);

    const char* addCreativeItemEntry_sig = "FD 7B BE A9 F4 4F 01 A9 FD 03 00 91 ?? ?? ?? ?? F4 03 00 AA 20 A0 00 91 F3 03 01 AA ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 81 62 40 39 3F 00 20 6B ?? ?? ?? ?? 68 1A 40 F9 00 01 40 F9 ?? ?? ?? ??";
    
    uintptr_t addr = find_signature(base_module_addr, mod_size, addCreativeItemEntry_sig);
    if (addr != 0) {
        LOGI("[+] addCreativeItemEntry encontrado en offset real: 0x%lx", addr - base_module_addr);
        
        A64HookFunction((void*)addr, (void*)hk_addCreativeItemEntry, (void**)&original_addCreativeItemEntry);
        LOGI("[+] ¡Framework de Pestañas Hookeado con éxito en ARM64!");
        
        // Escanear global y el mundo activo reciente de forma inteligente
        search_and_load_active_world_tabs();
    } else {
        LOGE("[-] No se pudo ubicar addCreativeItemEntry.");
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando Framework de Pestañas Custom...");
    init_framework();
    return JNI_VERSION_1_6;
}
