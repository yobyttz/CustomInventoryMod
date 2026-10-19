#include <ll/api/mod/NativeMod.hpp>
#include <ll/api/memory/Hook.hpp>
#include <ll/api/Logger.hpp>

class CustomInventoryMod {
public:
    static CustomInventoryMod& getInstance() {
        static CustomInventoryMod instance;
        return instance;
    }

    bool load() {
        getSelf().getLogger().info("¡Mod de Custom Inventory cargado correctamente!");
        return true;
    }

    bool enable() {
        getSelf().getLogger().info("Activando hooks para el controlador de inventario...");
        return true;
    }

    bool disable() {
        return true;
    }

    ll::mod::NativeMod& getSelf() const {
        return *ll::mod::NativeMod::current();
    }
};

PL_REGISTER_MOD(CustomInventoryMod, CustomInventoryMod::getInstance());
