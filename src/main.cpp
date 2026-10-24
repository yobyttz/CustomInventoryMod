#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include "subhook.h"

#define TAG "CustomInventoryMod"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

typedef void* (*ContainerControllerInit_t)(void* self);
ContainerControllerInit_t original_controller_init = nullptr;

void* hooked_container_controller_init(void* self) {
    LOGI("[+] ¡Se interceptó el controlador de contenedores / inventario!");
    return original_controller_init(self);
}

uintptr_t get_module_base(const char* module_name) {
    void* handle = dlopen(module_name, RTLD_NOW);
    if (!handle) return 0;
    
    struct link_map* map = nullptr;
    dlinfo(handle, RTLD_DI_LINKMAP, &map);
    dlclose(handle);
    
    return map ? reinterpret_cast<uintptr_t>(map->l_addr) : 0;
}

void init_hooks() {
    uintptr_t base_addr = 0;
    while (base_addr == 0) {
        base_addr = get_module_base("libminecraftpe.so");
        if (base_addr == 0) {
            usleep(100000);
        }
    }
    
    LOGI("[*] libminecraftpe.so encontrado en la base: 0x%lx", base_addr);

    // Aquí irá tu offset real más adelante
    uintptr_t target_offset = 0x123456; 
    void* target_func = reinterpret_cast<void*>(base_addr + target_offset);

    subhook_t hook = subhook_new(target_func, (void*)hooked_container_controller_init, SUBHOOK_64BIT_OFFSET);
    if (subhook_install(hook) < 0) {
        LOGE("[-] ¡Error al instalar el hook con Subhook!");
    } else {
        LOGI("[+] ¡Hook instalado exitosamente!");
        original_controller_init = (ContainerControllerInit_t)subhook_get_trampoline(hook);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("[*] Cargando mod nativo en LeviLauncher...");
    init_hooks();
    return JNI_VERSION_1_6;
}
