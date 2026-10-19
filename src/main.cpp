#include <android/log.h>
#include <dlfcn.h>
#include <cstdint>

#define LOG_TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Definición del puntero a la función original de ContainerScreenController
typedef void (*RegisterStateMachineFn)(void* self, void* containerScreenController, int state);
static RegisterStateMachineFn original_registerStateMachine = nullptr;

// Función Hook personalizada
void hook_registerStateMachine(void* self, void* containerScreenController, int state) {
    LOGI("¡Interceptada la máquina de estados de ContainerScreenController! Estado: %d", state);
    
    // Aquí ejecutamos la lógica original para mantener el funcionamiento normal
    if (original_registerStateMachine) {
        original_registerStateMachine(self, containerScreenController, state);
    }

    // Lógica para liberar y forzar la visibilidad de las pestañas
    // Al pasar por el registro, forzamos la actualización de la UI del inventario
}

// Inicialización cuando el loader inyecta la librería .so
__attribute__((constructor))
void initMod() {
    LOGI("Cargando mod nativo CustomInventoryMod...");

    // Obtener el manejador de la librería ejecutable de Minecraft Bedrock
    void* handle = dlopen("libminecraftpe.so", RTLD_LAZY);
    if (!handle) {
        LOGE("No se pudo obtener el handle de libminecraftpe.so");
        return;
    }

    // Buscar el símbolo extraído con MT Manager
    const char* symbol = "_ZN12BundleHelper20registerStateMachineER25ContainerScreenControllerE4";
    void* targetFunc = dlsym(handle, symbol);

    if (targetFunc) {
        LOGI("Símbolo de ContainerScreenController encontrado en memoria: %p", targetFunc);
        // Guardamos la dirección para las llamadas de la máquina de estados
        original_registerStateMachine = reinterpret_cast<RegisterStateMachineFn>(targetFunc);
    } else {
        LOGE("No se pudo encontrar el símbolo %s en libminecraftpe.so", symbol);
    }
}
