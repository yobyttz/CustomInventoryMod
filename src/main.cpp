#include <android/log.h>
#include <dlfcn.h>
#include <fstream>
#include <cstdint>

#define LOG_TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Función para guardar un texto de prueba en la carpeta de descargas
void guardarLogDePrueba(const char* mensaje) {
    // Guarda una nota en /sdcard/Download/mod_test.txt
    std::ofstream file("/sdcard/Download/mod_test.txt", std::ios::app);
    if (file.is_open()) {
        file << mensaje << "\n";
        file.close();
    }
}

__attribute__((constructor))
void initMod() {
    LOGI("Inicializando CustomInventoryMod...");
    guardarLogDePrueba("=== MOD CARGADO CON ÉXITO EN MINECRAFT ===");

    void* handle = dlopen("libminecraftpe.so", RTLD_LAZY);
    if (!handle) {
        guardarLogDePrueba("Error: No se pudo abrir libminecraftpe.so");
        return;
    }

    guardarLogDePrueba("Éxito: libminecraftpe.so encontrado en memoria.");

    const char* symbol = "_ZN12BundleHelper20registerStateMachineER25ContainerScreenControllerE4";
    void* targetFunc = dlsym(handle, symbol);

    if (targetFunc) {
        guardarLogDePrueba("Éxito: Símbolo de ContainerScreenController encontrado!");
    } else {
        guardarLogDePrueba("Error: No se encontró el símbolo en la memoria.");
    }
}
