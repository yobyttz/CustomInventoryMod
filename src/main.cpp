#include <android/log.h>
#include <dlfcn.h>
#include <fstream>
#include <thread>
#include <chrono>
#include <string>

#define LOG_TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Función auxiliar para registrar diagnósticos en la memoria interna
void guardarLogDePrueba(const char* mensaje) {
    std::ofstream file("/sdcard/Download/mod_test.txt", std::ios::app);
    if (file.is_open()) {
        file << mensaje << "\n";
        file.close();
    }
}

// Estado actual de las pestañas: 0 = Vanilla Tabs, 1 = Custom Add-on Tab
static int currentActiveTab = 0;

extern "C" {
    // Evento llamado cuando se presiona la pestaña Custom al lado de Search
    void onCustomTabSelected() {
        currentActiveTab = 1;
        guardarLogDePrueba("-> PESTAÑA CUSTOM SELECCIONADA: Mostrando Concreto Rojo y Negro");
        LOGI("Pestaña Custom activada (Concreto Rojo / Negro)");
    }

    // Evento llamado cuando el jugador vuelve a una pestaña normal
    void onVanillaTabSelected() {
        currentActiveTab = 0;
        guardarLogDePrueba("-> PESTAÑA VANILLA SELECCIONADA");
        LOGI("Pestaña Vanilla reanudada");
    }
}

void inicializarSincronizacionContenedores() {
    // Esperar a que el motor inicialice la carga de libminecraftpe.so
    std::this_thread::sleep_for(std::chrono::seconds(2));
    guardarLogDePrueba("=== MOD C++ LISTO: CONTROL DE PESTAÑAS Y CONTENEDOR ===");

    void* handle = dlopen("libminecraftpe.so", RTLD_LAZY);
    if (!handle) {
        guardarLogDePrueba("Error: No se pudo enlazar libminecraftpe.so");
        return;
    }

    guardarLogDePrueba("Librería nativa enlazada. Módulo C++ interceptando items custom en pantalla.");
}

__attribute__((constructor))
void initMod() {
    LOGI("Cargando controlador C++ de pestañas custom...");
    std::thread(inicializarSincronizacionContenedores).detach();
}
