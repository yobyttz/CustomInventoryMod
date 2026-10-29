#include "InventoryTab.hpp"
#include "Signatures.hpp"

#include <pl/ModMenu.hpp>
#include <pl/memory/Hook.hpp>
#include <cstdio>
#include <cstdarg>
#include <android/log.h>

namespace inventorytab {

static const char* LOG_FILE_PATH = "/storage/emulated/0/Download/mod_debug.txt";
static const char* TAG = "InventoryTab";

void write_log(const char* level, const char* fmt, ...) {
    va_list args1;
    va_start(args1, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, TAG, fmt, args1);
    va_end(args1);

    FILE* f = fopen(LOG_FILE_PATH, "a");
    if (f != nullptr) {
        fprintf(f, "[%s] ", level);
        va_list args2;
        va_start(args2, fmt);
        vfprintf(f, fmt, args2);
        va_end(args2);
        fprintf(f, "\n");
        fclose(f);
    }
}

#define LOGI(...) write_log("INFO", __VA_ARGS__)
#define LOGE(...) write_log("ERROR", __VA_ARGS__)

// ========== HOOKS ==========
using ContainerOpenFn = void(*)(void* self);
ContainerOpenFn originalContainerOpen = nullptr;

void hookedContainerOpen(void* self) {
    LOGI("¡ContainerScreenControllerOpen llamado! (Inventario abierto)");
    if (originalContainerOpen) originalContainerOpen(self);
}

using ContainerDtorFn = void(*)(void* self);
ContainerDtorFn originalContainerDtor = nullptr;

void hookedContainerDtor(void* self) {
    LOGI("ContainerScreenControllerDtor llamado (Inventario cerrado)");
    if (originalContainerDtor) originalContainerDtor(self);
}

using SlotSelectedFn = void(*)(void* self);
SlotSelectedFn originalSlotSelected = nullptr;

void hookedSlotSelected(void* self) {
    LOGI("OnContainerSlotSelected llamado");
    if (originalSlotSelected) originalSlotSelected(self);
}

// ========== CALLBACK ESTÁTICO (sin capturar this) ==========
static bool g_moduleEnabled = false;

static void onModuleToggle(std::string_view /*moduleId*/, bool enabled) {
    g_moduleEnabled = enabled;
    if (enabled) {
        LOGI("Módulo Custom Inventory Tab ACTIVADO");
    } else {
        LOGI("Módulo Custom Inventory Tab DESACTIVADO");
    }
}

// ========== MOD ==========
InventoryTabMod& InventoryTabMod::instance() {
    static InventoryTabMod mod;
    return mod;
}

void InventoryTabMod::registerModule() {
    // Ojo: NO capturar this en el lambda
    bool ok = pl::modmenu::ModuleBuilder("inventorytab.custom", "Custom Inventory Tab")
        .modId("inventorytab")
        .description("Tab personalizado en el inventario")
        .defaultEnabled(false)
        .onToggle(onModuleToggle)   // función estática, no lambda con this
        .registerModule();

    if (ok) {
        LOGI("Módulo registrado correctamente en el Mod Menu");
    } else {
        LOGE("Falló al registrar el módulo en el Mod Menu");
    }
}

void InventoryTabMod::installHooks() {
    auto openAddr = resolve(SignatureId::ContainerScreenControllerOpen);
    if (openAddr) {
        pl::memory::hook(
            reinterpret_cast<void*>(openAddr),
            reinterpret_cast<void*>(&hookedContainerOpen),
            reinterpret_cast<void**>(&originalContainerOpen),
            pl::memory::HookPriority::Normal
        );
        LOGI("Hook ContainerOpen instalado → 0x%lx", (unsigned long)openAddr);
    } else {
        LOGE("No se encontró signature de ContainerScreenControllerOpen");
    }

    auto dtorAddr = resolve(SignatureId::ContainerScreenControllerDtor);
    if (dtorAddr) {
        pl::memory::hook(
            reinterpret_cast<void*>(dtorAddr),
            reinterpret_cast<void*>(&hookedContainerDtor),
            reinterpret_cast<void**>(&originalContainerDtor),
            pl::memory::HookPriority::Normal
        );
        LOGI("Hook ContainerDtor instalado → 0x%lx", (unsigned long)dtorAddr);
    } else {
        LOGE("No se encontró signature de ContainerScreenControllerDtor");
    }

    auto slotAddr = resolve(SignatureId::ContainerScreenControllerOnContainerSlotSelected);
    if (slotAddr) {
        pl::memory::hook(
            reinterpret_cast<void*>(slotAddr),
            reinterpret_cast<void*>(&hookedSlotSelected),
            reinterpret_cast<void**>(&originalSlotSelected),
            pl::memory::HookPriority::Normal
        );
        LOGI("Hook OnContainerSlotSelected instalado → 0x%lx", (unsigned long)slotAddr);
    } else {
        LOGE("No se encontró signature de OnContainerSlotSelected");
    }
}

void InventoryTabMod::uninstallHooks() {
    LOGI("Desinstalando hooks...");
}

bool InventoryTabMod::load(pl::mod::ModContext& /*ctx*/) {
    FILE* f = fopen(LOG_FILE_PATH, "w");
    if (f) {
        fprintf(f, "=== InventoryTab Debug Log ===\n");
        fclose(f);
    }

    LOGI("Mod cargado correctamente");
    // NO registrar el módulo aquí
    return true;
}

bool InventoryTabMod::enable(pl::mod::ModContext& /*ctx*/) {
    LOGI("enable() llamado");

    // 1. Registrar el módulo del menú AQUÍ
    registerModule();

    // 2. Resolver signatures e instalar hooks
    LOGI("Intentando resolver signatures...");
    if (!resolveAll()) {
        LOGE("Falló al resolver las signatures");
        // Igual devolvemos true para que el mod no se deshabilite del todo
        return true;
    }

    LOGI("Signatures resueltas. Instalando hooks...");
    installHooks();
    LOGI("Mod habilitado con éxito");
    return true;
}

bool InventoryTabMod::disable(pl::mod::ModContext& /*ctx*/) {
    uninstallHooks();
    clear();
    LOGI("Mod deshabilitado");
    return true;
}

bool InventoryTabMod::unload(pl::mod::ModContext& /*ctx*/) {
    LOGI("Mod descargado");
    return true;
}

}
