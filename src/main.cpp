#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include "dobby.h"

#define LOG_TAG "NativeTabsMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Estructura para almacenar los ítems de las pestañas custom (98 y 99)
struct CustomTabsManager {
    std::vector<std::string> leftTabItems;
    std::vector<std::string> rightTabItems;

    static CustomTabsManager& getInstance() {
        static CustomTabsManager instance;
        return instance;
    }

    void init() {
        // Tus ítems específicos para la pestaña izquierda (98)
        leftTabItems = {"minecraft:diamond_block", "minecraft:gold_block", "minecraft:emerald_block"};
        
        // Tus ítems específicos para la pestaña derecha (99)
        rightTabItems = {"minecraft:diamond_sword", "minecraft:netherite_sword"};
        
        LOGI("[C++] Listas de ítems custom cargadas correctamente.");
    }
};

// Puntero para la función original de Bedrock
typedef void* (*GetCreativeCategory_t)(void* self, int categoryIndex);
GetCreativeCategory_t orig_getCreativeCategory = nullptr;

// Hook para interceptar cuando el juego pide las categorías del inventario
void* hook_getCreativeCategory(void* self, int categoryIndex) {
    if (categoryIndex == 98) {
        LOGI("[C++] Pestaña izquierda (98) interceptada con éxito.");
        // Aquí el juego consultará los datos de la pestaña 98
    } else if (categoryIndex == 99) {
        LOGI("[C++] Pestaña derecha (99) interceptada con éxito.");
        // Aquí el juego consultará los datos de la pestaña 99
    }
    
    if (orig_getCreativeCategory) {
        return orig_getCreativeCategory(self, categoryIndex);
    }
    return nullptr;
}

void setupHooks() {
    LOGI("[C++] Buscando símbolo CreativeItemRegistry en libminecraftpe.so...");
    
    // Resolver el símbolo en memoria del juego para aplicar Dobby
    void* symbol = DobbySymbolResolver("libminecraftpe.so", "_ZN20CreativeItemRegistry19getCreativeCategoryEi");
    if (symbol) {
        DobbyHook(symbol, (dobby_dummy_func_t)hook_getCreativeCategory, (dobby_dummy_func_t*)&orig_getCreativeCategory);
        LOGI("[C++] ¡Hook aplicado exitosamente con Dobby en libminecraftpe.so!");
    } else {
        LOGE("[C++] No se pudo resolver el símbolo para el hook.");
    }
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("==================================================");
    LOGI("   Iniciando Mod Nativo con Hooks (1.26.52.3)     ");
    LOGI("==================================================");

    CustomTabsManager::getInstance().init();
    setupHooks();

    return JNI_VERSION_1_6;
}
