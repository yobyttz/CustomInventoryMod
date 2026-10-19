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

// Control de página activa: 0 = Pestañas Default Vanilla, 1 = Pestañas Custom Add-ons
static int currentPageIndex = 0;

extern "C" {
    // Función de cambio de página invocada por la interacción de UI
    void toggleCreativeTabPages() {
        currentPageIndex = (currentPageIndex == 0) ? 1 : 0;
        
        if (currentPageIndex == 1) {
            guardarLogDePrueba("-> ESTADO CAMBIADO: Mostrando Pestañas Custom (Add-ons)");
        } else {
            guardarLogDePrueba("-> ESTADO CAMBIADO: Mostrando Pestañas Predeterminadas (Vanilla)");
        }
    }
}

void escucharEventosUI() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    guardarLogDePrueba("=== MOD LISTO PARA INTERCEPTAR BOTÓN DE PESTAÑAS ===");
    guardarLogDePrueba("Escuchando evento: button.custom_tab_page_toggle");
}

__attribute__((constructor))
void initMod() {
    LOGI("Inicializando módulo de Custom Tabs...");
    std::thread(escucharEventosUI).detach();
}
