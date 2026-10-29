#pragma once
#include <pl/Mod.hpp>

namespace inventorytab {

class InventoryTabMod {
public:
    static InventoryTabMod& instance();

    bool load(pl::mod::ModContext& ctx);
    bool enable(pl::mod::ModContext& ctx);
    bool disable(pl::mod::ModContext& ctx);
    bool unload(pl::mod::ModContext& ctx);

private:
    bool mEnabled = false;

    void registerModule();
    void installHooks();
    void uninstallHooks();
};

}
