#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace inventorytab {

struct HashedStringExact {
    std::uint64_t hash{};
    std::string text;
    void* lastMatch{nullptr};
    explicit HashedStringExact(std::string value);
};

struct EngineApi {
    // firmas alineadas al motor (Android arm64)
    using GetCreativeCategoryFn = void* (*)(void* registry, std::uint8_t category);
    using AddChildGroupFn = void* (*)(void* category, HashedStringExact& name, void* iconItemInstance);
    using NewItemEntryFn = void* (*)(void* registry, std::uint32_t netId, void* itemInstance);
    using AddCreativeItemEntryFn = void (*)(void* group, void* entry);
    using UpdateNetIdMapFn = void (*)(void* registry);
    using ItemInstanceNameCtorFn = void (*)(void* self, char const* name, std::size_t len, int count, int aux, void* userData);
    using ItemInstanceDtorFn = void (*)(void* self);
    using ItemGetItemFn = void* (*)(void* itemInstance);
    using AddLooseCreativeItemsFn = void (*)(void* initializer, void const* worldVersion);

    void* addLooseCreativeItemsTarget{nullptr};
    AddLooseCreativeItemsFn addLooseCreativeItemsOriginal{nullptr};

    GetCreativeCategoryFn getCreativeCategory{nullptr};
    AddChildGroupFn addChildGroup{nullptr};
    NewItemEntryFn newItemEntry{nullptr};
    AddCreativeItemEntryFn addCreativeItemEntry{nullptr};
    UpdateNetIdMapFn updateNetIdMap{nullptr};
    ItemInstanceNameCtorFn itemInstanceNameCtor{nullptr};
    ItemInstanceDtorFn itemInstanceDtor{nullptr};
    ItemGetItemFn itemGetItem{nullptr};

    bool resolve();
    void clear();
};

} // namespace inventorytab
