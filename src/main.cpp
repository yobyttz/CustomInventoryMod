#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>

#define LOG_TAG "NativeTabsMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Estructura base para inicializar el mod en C++
struct CustomTabsManager {
    std::vector<std::string> leftTabItems;
    std::vector<std::string> rightTabItems;

    static CustomTabsManager& getInstance() {
        static CustomTabsManager instance;
        return instance;
    }

    void init() {
        leftTabItems = {"minecraft:diamond_block", "minecraft:gold_block"};
        rightTabItems = {"minecraft:diamond_sword"};
        LOGI("[C++] Mod nativo inicializado correctamente para v1.26.52.3.");
    }
};

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("Cargando libCustomInventoryMod.so...");
    CustomTabsManager::getInstance().init();
    return JNI_VERSION_1_6;
}
