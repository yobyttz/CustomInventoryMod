#include <android/log.h>
#include <dlfcn.h>
#include <fstream>
#include <thread>
#include <chrono>

#define LOG_TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

void guardarLogDePrueba(const char* mensaje) {
    std::ofstream file("/sdcard/Download/mod_test.txt", std::ios::app);
    if (file.is_open()) {
        file << mensaje << "\n";
        file.close();
    }
}

// Variable que rastrea qué página de pestañas está activa (0 = Vanilla, 1 = Addons Custom)
static int currentPageIndex = 0;

extern "C" {
    // Función expuesta para cambiar de página de pestañas
    void toggleCreativeTabPages() {
        currentPageIndex = (currentPageIndex == 0) ? 1 : 0;
        LOGI("Página de pestañas cambiada a: %d", currentPageIndex);
    }
}

void activarLiberadorDeTabs() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    guardarLogDePrueba("=== INICIALIZANDO LIBERADOR DE PESTAÑAS CREATIVAS ===");

    void* handle = dlopen("libminecraftpe.so", RTLD_LAZY);
    if (!handle) {
        guardarLogDePrueba("Error al enlazar libminecraftpe.so");
        return;
    }

    guardarLogDePrueba("Librería de Minecraft enlazada con éxito.");
    guardarLogDePrueba("Estado: Sistema listo para procesar Custom Tabs y navegación por flecha.");
}

__attribute__((constructor))
void initMod() {
    LOGI("Cargando Liberador de Tabs Creativas...");
    std::thread(activarLiberadorDeTabs).detach();
}
