#include "Signatures.hpp"
#include <array>
#include <string>
#include <vector>
#include <pl/memory/Signature.hpp>

namespace inventorytab {
namespace {

struct SignatureDefinition {
    SignatureId id;
    std::string_view pattern;
};

std::array<std::uintptr_t, static_cast<std::size_t>(SignatureId::Count)> addresses{};

const std::array<SignatureDefinition, static_cast<std::size_t>(SignatureId::Count)> definitions{{
    {SignatureId::ContainerScreenControllerOpen,
     "? ? ? A9 ? ? ? F9 FD 03 00 91 F3 03 00 AA ? ? ? ? ? ? ? F9 E1 03 1F 2A ? ? ? ? ? ? ? F9 ? ? ? F9"},

    {SignatureId::ContainerScreenControllerDtor,
     "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 56 D0 3B D5 F3 03 00 AA ? ? ? F9 ? ? ? F9 ? ? ? ? ? ? ? 91 ? ? ? F9 ? ? ? F9 ? ? ? 91 ? ? ? F9 ? ? ? ? ? ? ? F9"},

    {SignatureId::ContainerScreenControllerOnContainerSlotSelected,
     "FD 7B BF A9 FD 03 00 91 08 00 40 F9 08 E9 40 F9 00 01 3F D6 E0 03 1F 2A FD 7B C1 A8 C0 03 5F D6"},

    {SignatureId::ContainerScreenControllerGetItemStack,
     "? ? ? D1 ? ? ? A9 ? ? ? A9 ? ? ? 91 54 D0 3B D5 F3 03 00 AA ? ? ? 91 ? ? ? F9 ? ? ? F8 ? ? ? ? ? ? ? F9 ? ? ? F9 ? ? ? 91 ? ? ? 91"},

    {SignatureId::ContainerScreenControllerHandleSelectSlot,
     "FD 7B BD A9 F6 57 01 A9 F4 4F 02 A9 FD 03 00 91 F3 03 00 AA 00 E0 31 91 F5 03 02 2A F6 03 01 AA ? ? ? ? 28 00 80 52 69 02 40 F9 E0 03 13 AA 68 AA 35 39 28 ED 40 F9 00 01 3F D6 02 18 40 B9 E1 03 00 AA E0 03 13 AA ? ? ? ? 08 00 40 F9 F4 03 00 AA 08 19 40 F9 00 01 3F D6 ? ? ? ?"},

    {SignatureId::ContainerScreenControllerGetSelectedSlotInfo,
     "FD 7B BD A9 F5 0B 00 F9 F4 4F 02 A9 FD 03 00 91 ? ? ? ? ? ? ? ? F4 03 00 AA 22 18 40 B9 ? ? ? ? 93 E2 31 91 F5 03 00 AA E0 03 13 AA ? ? ? ? ? ? ? ? E0 03 15 AA ? ? ? ? ? ? ? ? 88 A2 6F 39 89 FA 45 F9 01 00 80 12 0A FD 41 D3 1F 01 00 72 48 01 89 9A ? ? ? ? 88 02 4C B9 1F 05 00 31"},

    {SignatureId::ScreenViewRender,
     "? ? ? FC ? ? ? 6D ? ? ? 6D ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? A9 ? ? ? 91 ? ? ? D1 48 D0 3B D5 F3 03 00 AA"},
}};

}

bool resolveAll(std::string_view libraryName) {
    const std::string library(libraryName);
    std::vector<std::string> patterns;
    patterns.reserve(definitions.size());

    for (const auto& def : definitions) {
        if (!def.pattern.empty())
            patterns.emplace_back(def.pattern);
    }

    const auto resolved = pl::memory::resolveSignatures(patterns, library.c_str());
    addresses.fill(0);

    bool any = false;
    for (const auto& def : definitions) {
        std::uintptr_t addr = 0;
        if (!def.pattern.empty()) {
            auto it = resolved.find(std::string(def.pattern));
            if (it != resolved.end()) addr = it->second;
        }
        addresses[static_cast<std::size_t>(def.id)] = addr;
        any = any || (addr != 0);
    }
    return any;
}

std::uintptr_t resolve(SignatureId id) {
    auto index = static_cast<std::size_t>(id);
    return index < addresses.size() ? addresses[index] : 0;
}

void clear() {
    addresses.fill(0);
}

}
