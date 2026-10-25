#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <cstdint>
#include <cstdlib>
#include "subhook.h" 

#define LOG_TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Función auxiliar para obtener la dirección base de libminecraftpe.so en memoria
uintptr_t get_module_base(const char* module_name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    
    uintptr_t addr = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name)) {
            char* sep = strchr(line, '-');
            if (sep) {
                *sep = '\0';
                addr = strtoull(line, nullptr, 16);
                break;
            }
        }
    }
    fclose(fp);
    return addr;
}

// Puntero del hook usando la estructura correcta de Subhook
subhook_t container_hook = nullptr;
typedef void* (*ContainerScreenController_t)(void* _this);
ContainerScreenController_t original_ContainerScreenController = nullptr;

// Nuestra función "hook" que interceptará la original y creará el archivo en Descargas
void* hooked_ContainerScreenController(void* _this) {
    LOGI("[+] ¡Se interceptó ContainerScreenController con éxito!");
    
    // Escribimos en el archivo de texto en la carpeta Download tal como lo hacíamos antes
    FILE* f = fopen("/storage/emulated/0/Download/mod_debug.txt", "a");
    if (f) {
        fprintf(f, "[+] ¡Inventario interceptado con éxito en ContainerScreenController!\n");
        fclose(f);
    }
    
    // Llamamos a la función original usando el trampoline para no romper el flujo del juego
    if (original_ContainerScreenController) {
        return original_ContainerScreenController(_this);
    }
    return nullptr;
}

// Función principal de inicialización que se carga al iniciar la librería .so
void initialize_mod() {
    LOGI("[*] Iniciando CustomInventoryMod...");

    uintptr_t base_addr = 0;
    // Esperamos a que libminecraftpe.so esté completamente cargado en memoria
    while (base_addr == 0) {
        base_addr = get_module_base("libminecraftpe.so");
        if (base_addr == 0) {
            usleep(100000); // Esperar 100ms antes de reintentar
        }
    }

    LOGI("[+] libminecraftpe.so encontrado en la base: 0x%lx", base_addr);

    // Offset exacto descubierto mediante escaneo local en Termux
    uintptr_t target_function_offset = 0x1dc7ac0;
    void* target_func = reinterpret_cast<void*>(base_addr + target_function_offset);

    LOGI("[*] Intentando enganchar la función en la dirección: %p", target_func);

    // Instalación formal usando subhook_new e instalando el hook directamente
    container_hook = subhook_new(target_func, (void*)hooked_ContainerScreenController, 0);
    if (container_hook != nullptr) {
        if (subhook_install(container_hook) >= 0) {
            original_ContainerScreenController = (ContainerScreenController_t)subhook_get_trampoline(container_hook);
            LOGI("[+] ¡Hook instalado correctamente con Subhook!");
        } else {
            LOGE("[-] Error al instalar el hook con subhook_install.");
        }
    } else {
        LOGE("[-] Error al crear el hook con subhook_new.");
    }
}

// Punto de entrada estándar para la inyección nativa
__attribute__((constructor)) void entry() {
    initialize_mod();
}
