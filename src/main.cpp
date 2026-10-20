#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>

#define LOG_TAG "NativeTabsMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


struct CustomTabsManager {
    std::vector<std::string> leftTabItems;
    std::vector<std::string> rightTabItems;

    static CustomTabsManager& getInstance() {
        static CustomTabsManager instance;
        return instance;
    }

    void initializeDefaultItems() {
        // Agregamos ítems de prueba para verificar que el panel gris los renderice
        leftTabItems.push_back("minecraft:diamond_block");
        leftTabItems.push_back("minecraft:gold_block");
        leftTabItems.push_back("minecraft:emerald_block");

        rightTabItems.push_back("minecraft:diamond_sword");
        rightTabItems.push_back("minecraft:netherite_sword");

        LOGI("[C++] Ítems por defecto cargados en las colecciones custom.");
    }
};


void hookCreativeRegistry() {
    LOGI("[C++] Configurando ganchos para CreativeItemRegistry en libminecraftpe.so...");
    
    
}


extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("==================================================");
    LOGI("   Iniciando Mod Nativo de Pestañas (1.26.52.3)    ");
    LOGI("==================================================");

    
    CustomTabsManager::getInstance().initializeDefaultItems();

    
    hookCreativeRegistry();

    return JNI_VERSION_1_6;
}
