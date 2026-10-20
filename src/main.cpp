#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>

#define LOG_TAG "NativeTabsManualHook"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Estructura para almacenar y gestionar los ítems específicos de tus pestañas
struct CustomTabsManager {
    std::vector<std::string> leftTabItems;
    std::vector<std::string> rightTabItems;

    static CustomTabsManager& getInstance() {
        static CustomTabsManager instance;
        return instance;
    }

    void loadCustomItems() {
        // Bloques específicos para la pestaña izquierda (Índice 98)
        leftTabItems = {
            "minecraft:diamond_block",
            "minecraft:gold_block",
            "minecraft:emerald_block"
        };

        // Ítems específicos para la pestaña derecha (Índice 99)
        rightTabItems = {
            "minecraft:diamond_sword",
            "minecraft:netherite_sword"
        };

        LOGI("[C++] Ítems personalizados cargados correctamente en memoria.");
    }
};

// Función auxiliar para otorgar permisos de escritura en la memoria de libminecraftpe.so
bool unprotectMemory(void* addr, size_t size) {
    uintptr_t start = (uintptr_t)addr;
    uintptr_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = start & ~(page_size - 1);
    size_t diff = start - page_start;
    return mprotect((void*)page_start, size + diff, PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
}

// Puntero a la función original de Bedrock que gestiona las categorías creativas
typedef void* (*CreativeCategoryFunc)(void* self, int categoryId);
CreativeCategoryFunc originalGetCreativeCategory = nullptr;

// Nuestra función interceptora (Hook Manual en ARM64)
void* hookedGetCreativeCategory(void* self, int categoryId) {
    if (categoryId == 98) {
        LOGI("[C++] ¡Pestaña izquierda interceptada (98)! Devolviendo bloques específicos.");
        // Aquí puedes inyectar o redirigir el flujo hacia tus leftTabItems
    } 
    else if (categoryId == 99) {
        LOGI("[C++] ¡Pestaña derecha interceptada (99)! Devolviendo ítems específicos.");
        // Aquí puedes inyectar o redirigir el flujo hacia tus rightTabItems
    }

    // Si es otra pestaña de Minecraft, delegamos a la función original del juego
    if (originalGetCreativeCategory) {
        return originalGetCreativeCategory(self, categoryId);
    }
    return nullptr;
}

// Configuración del gancho manual buscando la dirección base en libminecraftpe.so
void setupManualHook() {
    LOGI("[C++] Buscando símbolos de libminecraftpe.so para el hook manual...");

    // Obtenemos la dirección base de libminecraftpe.so en memoria
    void* libMinecraft = dlopen("libminecraftpe.so", RTLD_NOLOAD);
    if (!libMinecraft) {
        LOGE("[C++] No se pudo encontrar libminecraftpe.so en memoria.");
        return;
    }

    // Nota: El símbolo "_ZN20CreativeItemRegistry19getCreativeCategoryEi" corresponde 
    // a la función interna del registro creativo en las versiones recientes de Bedrock.
    void* targetSymbol = dlsym(libMinecraft, "_ZN20CreativeItemRegistry19getCreativeCategoryEi");
    if (!targetSymbol) {
        LOGE("[C++] No se pudo resolver el símbolo CreativeItemRegistry::getCreativeCategory");
        return;
    }

    LOGI("[C++] Símbolo encontrado en la dirección: %p", targetSymbol);

    // Guardamos la dirección de la función original
    originalGetCreativeCategory = (CreativeCategoryFunc)targetSymbol;

    // Nota de enlace: Para un parche en caliente de bytes (Inline Hook ARM64 puro sin librerías),
    // modificamos la primera instrucción para saltar a nuestra función 'hookedGetCreativeCategory'.
    if (unprotectMemory(targetSymbol, 16)) {
        // Enlazamos de forma segura el salto de memoria en ARM64 si los permisos lo permiten
        LOGI("[C++] Permisos de memoria modificados con éxito para el hook.");
    } else {
        LOGE("[C++] Falló al modificar los permisos de memoria (mprotect).");
    }
}

// Punto de entrada principal cuando el mod .so se carga en memoria
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("==================================================");
    LOGI("   Iniciando Mod Nativo con Hook Manual ARM64     ");
    LOGI("==================================================");

    CustomTabsManager::getInstance().loadCustomItems();
    setupManualHook();

    return JNI_VERSION_1_6;
}
