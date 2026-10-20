#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>

#define LOG_TAG "NativeTabsMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Estructura de respaldo para registrar tus elementos personalizados
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
        LOGI("[C++] Estructura de pestañas personalizadas inicializada correctamente.");
    }
};

// Punto de entrada al cargar la librería .so en Minecraft
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("==================================================");
    LOGI("   Iniciando Mod Nativo Estable (v1.26.52.3)      ");
    LOGI("==================================================");

    CustomTabsManager::getInstance().init();

    return JNI_VERSION_1_6;
}
