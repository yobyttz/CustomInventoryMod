#include "CreativeTab.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>

#include <android/log.h>
#include <cstdio>
#include <cstdarg>

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
constexpr std::size_t kCreativeItemEntrySize = 0xB0;
constexpr std::size_t kCreativeRegistryItemsBeginOffset = 0x18;
constexpr std::size_t kCreativeRegistryItemsEndOffset = 0x20;
constexpr std::size_t kCreativeEntryNetIdOffset = 0x24;
constexpr std::size_t kCreativeInitializerRegistryOffset = 0x20;
constexpr std::uint8_t kCreativeCategoryItems = 4;

// Ítems de prueba en tu tab
constexpr std::string_view kTabItems[] = {
    "minecraft:diamond_sword",
    "minecraft:netherite_sword",
    "minecraft:trident",
};

struct alignas(16) ItemStorage {
    std::array<std::byte, kItemInstanceSize> bytes{};
};

template <class T>
T readAt(void const* base, std::size_t off) {
    T v{};
    if (!base) return v;
    std::memcpy(&v, static_cast<std::byte const*>(base) + off, sizeof(T));
    return v;
}

void* readPtr(void const* base, std::size_t off) {
    return readAt<void*>(base, off);
}

std::uint32_t nextNetId(void* registry) {
    auto* begin = static_cast<std::byte*>(readPtr(registry, kCreativeRegistryItemsBeginOffset));
    auto* end = static_cast<std::byte*>(readPtr(registry, kCreativeRegistryItemsEndOffset));
    if (!begin || !end || end < begin) return 1;
    auto bytes = static_cast<std::size_t>(end - begin);
    if (bytes % kCreativeItemEntrySize != 0) return 1;
    std::uint32_t maxId = 0;
    for (auto* e = begin; e < end; e += kCreativeItemEntrySize) {
        maxId = std::max(maxId, readAt<std::uint32_t>(e, kCreativeEntryNetIdOffset));
    }
    return maxId + 1;
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
    if (!mApi.getCreativeCategory || !mApi.addChildGroup || !mApi.newItemEntry ||
        !mApi.addCreativeItemEntry || !mApi.itemInstanceNameCtor || !mApi.itemInstanceDtor ||
        !mApi.itemGetItem || !mApi.updateNetIdMap) {
        clog("[ERROR] API incompleta, no inyecto");
        return;
    }

    mInjecting = true;

    auto* category = mApi.getCreativeCategory(mCreativeRegistry, kCreativeCategoryItems);
    if (!category) {
        clog("[ERROR] Categoría Items (4) no disponible");
        mInjecting = false;
        return;
    }

    // Icono del grupo: piedra
    ItemStorage icon{};
    mApi.itemInstanceNameCtor(icon.bytes.data(), "minecraft:stone", 15, 1, 0, nullptr);
    bool iconOk = mApi.itemGetItem(icon.bytes.data()) != nullptr;
    if (!iconOk) {
        clog("[ERROR] No se pudo crear icono stone");
        mApi.itemInstanceDtor(icon.bytes.data());
        mInjecting = false;
        return;
    }

    HashedStringExact name{std::string("Custom")};
    void* group = mApi.addChildGroup(category, name, icon.bytes.data());
    mApi.itemInstanceDtor(icon.bytes.data());

    if (!group) {
        clog("[ERROR] addChildGroup devolvió null");
        mInjecting = false;
        return;
    }
    clog("[INFO] Tab 'Custom' creada");

    std::uint32_t netId = nextNetId(mCreativeRegistry);
    std::size_t added = 0;

    for (auto id : kTabItems) {
        ItemStorage item{};
        mApi.itemInstanceNameCtor(item.bytes.data(), id.data(), id.size(), 1, 0, nullptr);
        if (!mApi.itemGetItem(item.bytes.data())) {
            clog("[ERROR] No se pudo construir %.*s", (int)id.size(), id.data());
            mApi.itemInstanceDtor(item.bytes.data());
            continue;
        }
        auto* entry = mApi.newItemEntry(mCreativeRegistry, netId++, item.bytes.data());
        if (!entry) {
            clog("[ERROR] newItemEntry falló para %.*s", (int)id.size(), id.data());
            mApi.itemInstanceDtor(item.bytes.data());
            continue;
        }
        mApi.addCreativeItemEntry(group, entry);
        mApi.itemInstanceDtor(item.bytes.data());
        ++added;
        clog("[INFO] Agregado: %.*s", (int)id.size(), id.data());
    }

    if (added > 0) {
        mApi.updateNetIdMap(mCreativeRegistry);
        clog("[INFO] updateNetIdMap ok, items=%zu", added);
    }

    mInjectedRegistry = mCreativeRegistry;
    mInjecting = false;
    clog("[INFO] Inyección Custom terminada");
}

} // namespace inventorytab
