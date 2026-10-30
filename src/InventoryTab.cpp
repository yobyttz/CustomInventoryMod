#include "InventoryTab.hpp"

#include <pl/memory/Hook.hpp>
#include <android/log.h>
#include <cstdio>
#include <cstdarg>

namespace inventorytab {

static const char* LOG_FILE = "/storage/emulated/0/Download/mod_debug.txt";

void mlog(const char* fmt, ...) {
    va_list a;
    va_start(a, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, "InventoryTab", fmt, a);
    va_end(a);
    FILE* f = fopen(LOG_FILE, "a");
    if (f) {
        va_list b;
        va_start(b, fmt);
        vfprintf(f, fmt, b);
        fprintf(f, "\n");
        va_end(b);
        fclose(f);
    }
}

InventoryTabMod* InventoryTabMod::sInstance = nullptr;

InventoryTabMod& InventoryTabMod::instance() {
    static InventoryTabMod mod;
    return mod;
}

void InventoryTabMod::addLooseDetour(void* initializer, void const* worldVersion) {
    auto* self = sInstance;
    if (!self || !self->mApi.addLooseCreativeItemsOriginal) return;

    // Vanilla primero
    self->mApi.addLooseCreativeItemsOriginal(initializer, worldVersion);
    // Después nuestra tab
    self->mTab.onCreativeInitializer(initializer, worldVersion);
}

bool InventoryTabMod::load(pl::mod::ModContext&) {
    FILE* f = fopen(LOG_FILE, "w");
    if (f) {
        fprintf(f, "=== InventoryTab / Custom Creative Tab ===\n");
        fclose(f);
    }
    mlog("[INFO] load ok");
    return true;
}

bool InventoryTabMod::enable(pl::mod::ModContext&) {
    mlog("[INFO] enable()");
    sInstance = this;

    if (!mApi.resolve()) {
        mlog("[ERROR] resolve falló");
        sInstance = nullptr;
        return false;
    }

    void* original = nullptr;
    int rc = pl::memory::hook(
        mApi.addLooseCreativeItemsTarget,
        reinterpret_cast<void*>(&InventoryTabMod::addLooseDetour),
        &original,
        pl::memory::HookPriority::Normal
    );
    if (rc != 0 || !original) {
        mlog("[ERROR] hook addLooseCreativeItems falló code=%d", rc);
        mApi.clear();
        sInstance = nullptr;
        return false;
    }

    mApi.addLooseCreativeItemsOriginal =
        reinterpret_cast<EngineApi::AddLooseCreativeItemsFn>(original);
    mHookInstalled = true;
    mlog("[INFO] hook instalado; entra a un mundo en creativo para ver la tab Custom");
    return true;
}

bool InventoryTabMod::disable(pl::mod::ModContext&) {
    if (mHookInstalled && mApi.addLooseCreativeItemsTarget) {
        pl::memory::unhook(
            mApi.addLooseCreativeItemsTarget,
            reinterpret_cast<void*>(&InventoryTabMod::addLooseDetour)
        );
        mHookInstalled = false;
    }
    mTab.reset();
    mApi.clear();
    sInstance = nullptr;
    mlog("[INFO] disable ok");
    return true;
}

bool InventoryTabMod::unload(pl::mod::ModContext& ctx) {
    disable(ctx);
    mlog("[INFO] unload ok");
    return true;
}

} // namespace inventorytab
