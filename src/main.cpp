#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>

#define LOG_TAG "NativeTabs12652"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Lista de ítems registrados para cada pestaña
std::vector<std::string> leftTabItems;
std::vector<std::string> rightTabItems;

// Método para agregar ítems de prueba si aún no hay addons cargados
void populateDefaultItems() {
    // Ejemplo: Si la lista está vacía, agregamos bloques/ítems de prueba o custom
    if (leftTabItems.empty()) {
        leftTabItems.push_back("minecraft:diamond_block");
        leftTabItems.push_back("minecraft:gold_block");
        leftTabItems.push_back("minecraft:netherite_block");
    }
    if (rightTabItems.empty()) {
        rightTabItems.push_back("minecraft:diamond_sword");
        rightTabItems.push_back("minecraft:netherite_sword");
    }
}

// Hook de memoria nativo en libminecraftpe.so
// Cuando la UI solicita los ítems de 'custom_left_items' o 'custom_right_items'
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("Cargando hooks de almacenamiento para pestañas personalizadas...");
    
    populateDefaultItems();

    // Aquí LeviLauncher/Dobby intercepta la función de filtrado de ítems del inventario creativo.
    return JNI_VERSION_1_6;
}
