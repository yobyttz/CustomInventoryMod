#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>

#define LOG_TAG "NativeTabs12652"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

struct TabStorage {
    std::vector<std::string> leftItems;
    std::vector<std::string> rightItems;

    static TabStorage& getInstance() {
        static TabStorage instance;
        return instance;
    }

    void registerCustomItem(const std::string& itemIdentifier) {
        // Ignorar vanilla items
        if (itemIdentifier.find("minecraft:") != 0) {
            // Ejemplo de filtrado directo en C++
            if (itemIdentifier.find("weapon") != std::string::npos || 
                itemIdentifier.find("tool") != std::string::npos ||
                itemIdentifier.find("sword") != std::string::npos) {
                
                rightItems.push_back(itemIdentifier);
                LOGI("[C++] Item vinculado a custom_right_items: %s", itemIdentifier.c_str());
            } else {
                leftItems.push_back(itemIdentifier);
                LOGI("[C++] Item vinculado a custom_left_items: %s", itemIdentifier.c_str());
            }
        }
    }
};

// Hook nativo para capturar el registro de ítems de addons
void onBedrockItemInit(const std::string& itemId) {
    TabStorage::getInstance().registerCustomItem(itemId);
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("=========================================================");
    LOGI(" Custom Tabs Native Mod (1.26.52.3) ");
    LOGI(" Integrado con @crafting.top_tab & tab_navigation_panel ");
    LOGI("=========================================================");

    // Aquí LeviLauncher aplica los hooks de memoria
    return JNI_VERSION_1_6;
}
