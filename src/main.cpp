#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include "dobby.h"

#define LOG_TAG "NativeTabsMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Estructura para gestionar la inyección de items
struct CustomTabsManager {
    std::vector<std::string> leftTabItems;
    std::vector<std::string> rightTabItems;

    static CustomTabsManager& getInstance() {
        static CustomTabsManager instance;
        return instance;
    }

    void init() {
        leftTabItems = {"minecraft:diamond_block", "minecraft:gold_block", "minecraft:emerald_block"};
        rightTabItems = {"minecraft:diamond_sword", "minecraft:netherite_sword"};
        LOGI("[C++] Listas de items cargadas correctamente.");
    }
};

// Puntero para guardar la función original del juego
typedef void* (*GetCreativeCategory_t)(void* self, int categoryIndex);
GetCreativeCategory_t orig_getCreativeCategory = nullptr;

// Hook que intercepta la consulta de categorías
void* hook_getCreativeCategory(void* self, int categoryIndex) {
    if (categoryIndex == 98) {
        LOGI("[C++] Pestaña personalizada izquierda (98) solicitada.");
    } else if (categoryIndex == 99) {
        LOGI("[C++] Pestaña personalizada derecha (99) solicitada.");
    }
    
    if (orig_getCreativeCategory) {
        return orig_getCreativeCategory(self, categoryIndex);
    }
    return nullptr;
}

void setupHooks() {
    LOGI("[C++] Inicializando Dobby para libminecraftpe.so...");
    
    // Intenta resolver el símbolo del registro creativo
    void* symbol = DobbySymbolResolver("libminecraftpe.so", "_ZN20CreativeItemRegistry19getCreativeCategoryEi");
    if (symbol) {
        DobbyHook(symbol, (dobby_dummy_func_t)hook_getCreativeCategory, (dobby_dummy_func_t*)&orig_getCreativeCategory);
        LOGI("[C++] Hook aplicado exitosamente con Dobby!");
    } else {
        LOGE("[C++] No se pudo resolver el símbolo en libminecraftpe.so");
    }
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("==================================================");
    LOGI("   Iniciando Mod Nativo con Dobby (1.26.52.3)     ");
    LOGI("==================================================");

    CustomTabsManager::getInstance().init();
    setupHooks();

    return JNI_VERSION_1_6;
}
