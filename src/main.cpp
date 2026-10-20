#include <fstream>
#include <string>

// Ruta del log que confirmaste que funciona
const char* LOG_PATH = "/sdcard/custom_tabs_debug.txt";

// Función para escribir en el archivo
void writeLog(const std::string& message) {
    std::ofstream logFile(LOG_PATH, std::ios::out | std::ios::app);
    if (logFile.is_open()) {
        logFile << message << "\n";
        logFile.close();
    }
}

// ===================================================================
// 1. ESTA FUNCIÓN SE EJECUTA AUTOMÁTICAMENTE AL CARGAR EL .SO
// ===================================================================
__attribute__((constructor))
void initMod() {
    // Sobrescribimos el archivo para saber que el mod arrancó con éxito
    std::ofstream logFile(LOG_PATH, std::ios::out | std::ios::trunc);
    if (logFile.is_open()) {
        logFile << "[MOD INIT] El mod C++ se cargo e inicializo correctamente.\n";
        logFile.close();
    }
}

// ===================================================================
// 2. FUNCIÓN DE HOOK (Captura el clic de los botones del JSON UI)
// ===================================================================
// Aquí interceptas los eventos de ScreenController
void handleUICommand(const std::string& commandName) {
    if (commandName == "button.custom_tab_0") {
        writeLog("[CLIC DETECTADO] Se presiono el boton: Pestaña 1 (button.custom_tab_0)");
    } 
    else if (commandName == "button.custom_tab_1") {
        writeLog("[CLIC DETECTADO] Se presiono el boton: Pestaña 2 (button.custom_tab_1)");
    }
}
