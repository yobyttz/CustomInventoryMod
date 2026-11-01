#include "CreativeTab.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>

#include <android/log.h>

namespace inventorytab {
namespace {

static const char* LOG_FILE = "/storage/emulated/0/Download/mod_debug.txt";

void clog(const char* fmt, ...) {
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

constexpr std::size_t kItemInstanceSize = 0x80;
constexpr std::size_t kCreativeInitializerRegistryOffset = 0x20;
constexpr std::uint8_t kCreativeCategoryItems = 4;

struct alignas(16) ItemStorage {
    std::array<std::byte, kItemInstanceSize> bytes{};
};

void* readPtr(void const* base, std::size_t off) {
    if (!base) return nullptr;
    void* p = nullptr;
    std::memcpy(&p, static_cast<std::byte const*>(base) + off, sizeof(p));
    return p;
}

} // namespace

void CreativeTab::onCreativeInitializer(void* initializer, void const* worldVersion) {
    if (!initializer || !worldVersion) return;

    auto* registry = readPtr(initializer, kCreativeInitializerRegistryOffset);
    if (!registry) {
        clog("[ERROR] CreativeItemRegistry null en initializer+0x20");
        return;
    }

    std::lock_guard lock(mMutex);
    if (mCreativeRegistry != registry) {
        mCreativeRegistry = registry;
        mInjectedRegistry = nullptr;
    }
    tryInject();
}

void CreativeTab::reset() {
    std::lock_guard lock(mMutex);
    mCreativeRegistry = nullptr;
    mInjectedRegistry = nullptr;
    mInjecting = false;
}

void CreativeTab::tryInject() {
    if (mInjecting || !mCreativeRegistry) return;
    if (mInjectedRegistry == mCreativeRegistry) return;

    if (!mApi.getCreativeCategory || !mApi.addChildGroup ||
        !mApi.itemInstanceNameCtor || !mApi.itemInstanceDtor || !mApi.itemGetItem) {
        clog("[ERROR] API incompleta");
        return;
    }

    mInjecting = true;
    clog("[INFO] tryInject inicio registry=%p", mCreativeRegistry);

    auto* category = mApi.getCreativeCategory(mCreativeRegistry, kCreativeCategoryItems);
    clog("[INFO] category=%p", category);
    if (!category) {
        clog("[ERROR] Categoría Items (4) null");
        mInjecting = false;
        return;
    }

    ItemStorage icon{};
    clog("[INFO] construyendo icono stone...");
    mApi.itemInstanceNameCtor(icon.bytes.data(), "minecraft:stone", 15, 1, 0, nullptr);
    void* iconItem = mApi.itemGetItem(icon.bytes.data());
    clog("[INFO] icon Item*=%p", iconItem);

    if (!iconItem) {
        clog("[ERROR] icon stone falló");
        mApi.itemInstanceDtor(icon.bytes.data());
        mInjecting = false;
        return;
    }

    HashedStringExact name{std::string("Custom")};
    clog("[INFO] addChildGroup...");
    void* group = mApi.addChildGroup(category, name, icon.bytes.data());
    clog("[INFO] group=%p", group);

    mApi.itemInstanceDtor(icon.bytes.data());
    clog("[INFO] icon destruido");

    if (!group) {
        clog("[ERROR] group null");
        mInjecting = false;
        return;
    }

    // SIN ítems todavía
    mInjectedRegistry = mCreativeRegistry;
    mInjecting = false;
    clog("[INFO] Tab vacía OK — sin ítems, sin updateNetIdMap");
}

} // namespace inventorytab
