#pragma once

#include "EngineApi.hpp"
#include "CreativeTab.hpp"
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
    EngineApi mApi;
    CreativeTab mTab{mApi};
    bool mHookInstalled{false};

    static InventoryTabMod* sInstance;
    static void addLooseDetour(void* initializer, void const* worldVersion);
};

} // namespace inventorytab
