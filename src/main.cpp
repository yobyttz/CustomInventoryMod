#include <android/log.h>
#include <dlfcn.h>
#include <cstdint>

#define LOG_TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Definición del tipo de función original
typedef void (*RegisterStateMachineFn)(void* self, void* containerScreenController, int state);
static RegisterStateMachineFn original_registerStateMachine = nullptr;

// Hook modificado para interactuar con los estados de las pestañas
void hook_registerStateMachine(void* self, void* containerScreenController, int state) {
    LOGI("ContainerScreenController interceptado -> self: %p, controller: %p, state: %d", self, containerScreenController, state);

    // Aquí es donde podemos alterar el comportamiento de las pestañas.
    // Al forzar o modificar valores del estado (state) o interactuar con el controlador,
    // podemos evitar que se bloqueen las pestañas adicionales o personalizadas.
    
    // Ejemplo de manipulación de estado para pruebas de liberación:
    // Si el juego pasa un estado restrictivo, podemos normalizarlo o forzar un flag abierto.
    int modifiedState = state;
    
    // Si ejecutamos la función original con el estado original o modificado:
    if (original_registerStateMachine) {
        original_registerStateMachine(self, containerScreenController, modifiedState);
    }
}

// Nota: Para interceptar de verdad en tiempo de ejecución de manera limpia sin desbordar,
// aseguramos el enlace dinámico y la preparación del hook con dlsym.
__attribute__((constructor))
void initMod() {
    LOGI("Inicializando CustomInventoryMod v2...");

    void* handle = dlopen("libminecraftpe.so", RTLD_LAZY);
    if (!handle) {
        LOGE("No se pudo abrir libminecraftpe.so");
        return;
    }

    const char* symbol = "_ZN12BundleHelper20registerStateMachineER25ContainerScreenControllerE4";
    void* targetFunc = dlsym(handle, symbol);

    if (targetFunc) {
        LOGI("Símbolo localizado con éxito en: %p", targetFunc);
        original_registerStateMachine = reinterpret_cast<RegisterStateMachineFn>(targetFunc);
        
        // En este punto, si utilizas un framework de hooking básico o un parche de desvío de funciones 
        // (como Mobby/Substrate adaptado a Android), redirigirías 'targetFunc' hacia 'hook_registerStateMachine'.
        // Como estamos cargando la base y validando la estabilidad con Levi, el siguiente paso 
        // es asegurar el puntero del hook en memoria.
    } else {
        LOGE("No se encontró el símbolo en memoria: %s", symbol);
    }
}
