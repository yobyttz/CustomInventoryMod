#include <android/log.h>
#include <dlfcn.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <string>

#define LOG_TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Función auxiliar para depuración segura en almacenamiento interno
void escribirLog(const char* mensaje) {
    std::ofstream file("/sdcard/Download/mod_debug.txt", std::ios::app);
    if (file.is_open()) {
        file << mensaje << "\n";
        file.close();
    }
}

// Estado del mod
static int activeTabState = 0; // 0 = Vanilla, 1 = Custom

extern "C" {
    // Esta función es llamada desde el JSON UI cuando el usuario presiona el botón custom
    void onCustomTabSelected() {
        activeTabState = 1;
        escribirLog("-> Acción: Pestaña Custom activada de forma segura.");
        LOGI("Pestaña Custom activada.");
    }

    // Esta función maneja el retorno a la pestaña vanilla
    void onVanillaTabSelected() {
        activeTabState = 0;
        escribirLog("-> Acción: Pestaña Vanilla reanudada.");
        LOGI("Pestaña Vanilla activada.");
    }
}

// Hilo de inicialización segura para evitar colisiones en el arranque
void inicializarModNativo() {
    // Damos un margen para que el motor de Bedrock cargue sus clases base de UI
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    escribirLog("=== INICIALIZANDO MOD C++ (LEVI LAUNCHER) ===");

    void* handle = dlopen("libminecraftpe.so", RTLD_LAZY);
    if (!handle) {
        LOGE("No se pudo obtener el handle de libminecraftpe.so");
        escribirLog("Error crítico: No se pudo enlazar libminecraftpe.so");
        return;
    }

    escribirLog("Librería compartida enlazada correctamente. Mod listo para operar.");
    LOGI("Mod de inventario inicializado sin errores de punteros.");
}

// Constructor principal que se ejecuta al inyectar la librería .so
__attribute__((constructor))
void entryPoint() {
    LOGI("Inyectando mod en el espacio de memoria de Minecraft...");
    // Lanzamos la inicialización en un hilo separado para no congelar el hilo principal gráfico
    std::thread(inicializarModNativo).detach();
}
