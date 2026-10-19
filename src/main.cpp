#include <android/log.h>
#include <dlfcn.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <string>

#define LOG_TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

void guardarLogDePrueba(const char* mensaje) {
    std::ofstream file("/sdcard/Download/mod_test.txt", std::ios::app);
    if (file.is_open()) {
        file << mensaje << "\n";
        file.close();
    }
}

void escanearSimbolos() {
    // Esperar 4 segundos a que Minecraft cargue por completo
    std::this_thread::sleep_for(std::chrono::seconds(4));
    
    guardarLogDePrueba("=== INICIANDO BÚSQUEDA DE VARIANTES ===");

    void* handle = dlopen("libminecraftpe.so", RTLD_LAZY);
    if (!handle) {
        guardarLogDePrueba("Error: No se pudo abrir libminecraftpe.so");
        return;
    }

    // Lista de posibles variantes o símbolos relacionados con el controlador de inventario
    const char* variantes[] = {
        "_ZN12BundleHelper20registerStateMachineER25ContainerScreenControllerE4",
        "_ZN25ContainerScreenController",
        "_ZN25ContainerScreenControllerC2Ev",
        "_ZN25ContainerScreenController18_handleClick",
        "_ZN25ContainerScreenController25addItemCollection",
        "BundleHelper"
    };

    for (const char* sym : variantes) {
        void* addr = dlsym(handle, sym);
        if (addr) {
            std::string msg = "¡ENCONTRADO!: " + std::string(sym) + " en " + std::to_string(reinterpret_cast<uintptr_t>(addr));
            guardarLogDePrueba(msg.c_str());
        } else {
            std::string msg = "No existe: " + std::string(sym);
            guardarLogDePrueba(msg.c_str());
        }
    }
    
    guardarLogDePrueba("=== FIN DEL ESCANEO ===");
}

__attribute__((constructor))
void initMod() {
    LOGI("Inicializando escáner de símbolos...");
    std::thread(escanearSimbolos).detach();
}
