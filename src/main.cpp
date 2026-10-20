#include <fstream>
#include <string>
#include <iostream>

// Ruta del archivo de prueba (puedes cambiarla a una ruta accesible en Android)
const std::string LOG_FILE_PATH = "/sdcard/custom_tabs_debug.txt";

// 1. Función que se ejecuta cuando el mod C++ se carga al iniciar
void onModLoaded() {
    std::ofstream logFile(LOG_FILE_PATH, std::ios::out | std::ios::trunc);
    if (logFile.is_open()) {
        logFile << "[MOD INIT] El mod C++ se ha cargado correctamente.\n";
        logFile.close();
    }
}

// 2. Función que escribe en el archivo cuando se detecta el clic de un botón
void logButtonPress(const std::string& buttonName) {
    std::ofstream logFile(LOG_FILE_PATH, std::ios::out | std::ios::app);
    if (logFile.is_open()) {
        logFile << "[BUTTON PRESS] Evento detectado: " << buttonName << "\n";
        logFile.close();
    }
}

// Hook de la pantalla donde LeviLauncher intercepta las acciones de la UI
void handleScreenCommand(const std::string& commandName) {
    // Si el comando recibido es uno de nuestros botones del JSON UI:
    if (commandName == "button.custom_tab_0") {
        logButtonPress("Pestaña 1 (button.custom_tab_0)");
    } 
    else if (commandName == "button.custom_tab_1") {
        logButtonPress("Pestaña 2 (button.custom_tab_1)");
    }
}
