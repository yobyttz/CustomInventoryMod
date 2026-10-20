#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <dlfcn.h>

#define LOG_TAG "NativeTabsManualHook"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Estructura para almacenar y gestionar los ítems específicos de tus pestañas
struct CustomTabsManager {
    std::vector<std::string> leftTabItems;
    std::vector<std::string> rightTabItems;

    static CustomTabsManager& getInstance() {
        static CustomTabsManager instance;
        return instance;
    }

    void loadCustomItems() {
        leftTabItems = {
            "minecraft:diamond_block",
            "minecraft:gold_block",
            "minecraft:emerald_block"
        };

        rightTabItems = {
            "minecraft:diamond_sword",
            "minecraft:netherite_sword"
        };

        LOGI("[C++] Ítems personalizados cargados correctamente en memoria.");
    }
};

// Función principal de inicialización sin fallos de símbolos externos
void setupManualHook() {
    LOGI("[C++] Inicializando gestor de pestañas nativas...");
    
    // Verificamos la carga segura de libminecraftpe.so en memoria de Android
    void* libMinecraft = dlopen("libminecraftpe.so", RTLD_NOLOAD | RTLD_LAZY);
    if (libMinecraft) {
        LOGI("[C++] libminecraftpe.so detectado en memoria correctamente.");
    } else {
        LOGI("[C++] libminecraftpe.so se cargará en runtime por LeviLauncher.");
    }
}

// Punto de entrada principal cuando el mod .so se carga en memoria
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("==================================================");
    LOGI("   Iniciando Mod Nativo Estable (1.26.52.3)       ");
    LOGI("==================================================");

    CustomTabsManager::getInstance().loadCustomItems();
    setupManualHook();

    return JNI_VERSION_1_6;
}
