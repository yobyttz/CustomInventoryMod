#include "EngineApi.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <link.h>
#include <string_view>

#include <android/log.h>
#include <cstdio>
#include <cstdarg>

namespace inventorytab {
namespace {

static const char* LOG_FILE = "/storage/emulated/0/Download/mod_debug.txt";

void elog(const char* fmt, ...) {
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

constexpr std::string_view kModuleName = "libminecraftpe.so";

struct ModuleView {
    std::uintptr_t base{};
    std::uintptr_t execStart{std::numeric_limits<std::uintptr_t>::max()};
    std::uintptr_t execEnd{};
};

bool isTarget(const char* path) {
    if (!path || !*path) return false;
    std::string_view name{path};
    return name.size() >= kModuleName.size() &&
           name.substr(name.size() - kModuleName.size()) == kModuleName;
}

int moduleCb(dl_phdr_info* info, std::size_t, void* opaque) {
    if (!info || !opaque || !isTarget(info->dlpi_name)) return 0;
    auto& out = *static_cast<ModuleView*>(opaque);
    out.base = static_cast<std::uintptr_t>(info->dlpi_addr);
    for (ElfW(Half) i = 0; i < info->dlpi_phnum; ++i) {
        const auto& ph = info->dlpi_phdr[i];
        if (ph.p_type != PT_LOAD || ph.p_memsz == 0) continue;
        if ((ph.p_flags & PF_X) == 0) continue;
        auto start = out.base + static_cast<std::uintptr_t>(ph.p_vaddr);
        auto end = start + static_cast<std::uintptr_t>(ph.p_memsz);
        out.execStart = std::min(out.execStart, start);
        out.execEnd = std::max(out.execEnd, end);
    }
    return 1;
}

bool findModule(ModuleView& out) {
    out = {};
    out.execStart = std::numeric_limits<std::uintptr_t>::max();
    dl_iterate_phdr(&moduleCb, &out);
    return out.base != 0 && out.execEnd > out.execStart;
}

int hexNibble(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

std::size_t sigLen(std::string_view sig) {
    std::size_t n = 0, pos = 0;
    while (pos < sig.size()) {
        while (pos < sig.size() && sig[pos] == ' ') ++pos;
        if (pos >= sig.size()) break;
        auto start = pos;
        while (pos < sig.size() && sig[pos] != ' ') ++pos;
        if (pos - start != 2) return 0;
        ++n;
    }
    return n;
}

bool matchRva(const ModuleView& m, std::uintptr_t rva, std::string_view sig) {
    auto len = sigLen(sig);
    if (!len) return false;
    auto addr = m.base + rva;
    if (addr < m.execStart || addr + len > m.execEnd) return false;
    auto* bytes = reinterpret_cast<const std::uint8_t*>(addr);
    std::size_t bi = 0, pos = 0;
    while (pos < sig.size()) {
        while (pos < sig.size() && sig[pos] == ' ') ++pos;
        if (pos >= sig.size()) break;
        char hi = sig[pos++], lo = sig[pos++];
        std::uint8_t value = 0, mask = 0;
        if (hi != '?') {
            int n = hexNibble(hi);
            if (n < 0) return false;
            value |= static_cast<std::uint8_t>(n << 4);
            mask |= 0xF0;
        }
        if (lo != '?') {
            int n = hexNibble(lo);
            if (n < 0) return false;
            value |= static_cast<std::uint8_t>(n);
            mask |= 0x0F;
        }
        if ((bytes[bi] & mask) != value) return false;
        ++bi;
    }
    return bi == len;
}

struct Target {
    const char* label;
    std::string_view signature;
    std::uintptr_t rva;
};

// Targets 1.26.52.3 (del set que encontraste)
constexpr Target kTargets[] = {
    {"addLooseCreativeItems",
     "FF 03 05 D1 FD 7B 0F A9 FC 67 10 A9 F8 5F 11 A9 F6 57 12 A9 F4 4F 13 A9 FD C3 03 91 55 D0 3B D5 B4 63 01 D1 F3 03 00 AA A8 16 40 F9 A8 83 1F F8 A0 83 1A F8 80 22 00 91 ?? ?? ?? ?? E8 23 00 91",
     0x0FFFF258},
    {"getCreativeCategory",
     "09 28 40 F9 ?? ?? ?? ?? 20 01 67 9E 28 1C 40 92 00 58 20 0E 00 38 30 2E 0C 00 26 1E 9F 05 00 F1 ?? ?? ?? ?? 2A 05 00 51 4A 01 08 8A ?? ?? ?? ?? 3F 01 08 EB EA 03 08 AA ?? ?? ?? ?? 2A 1D 00 12",
     0x0FBAA8B0},
    {"addChildGroup",
     "FF 03 02 D1 FD 7B 02 A9 FC 6F 03 A9 FA 67 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9 FD 83 00 91 59 D0 3B D5 28 17 40 F9 A8 83 1F F8 08 1C 40 F9 ?? ?? ?? ?? F4 03 00 AA 00 00 01 91 F5 03 02 AA",
     0x0FBAAC9C},
    {"newItemEntry",
     "FF 83 01 D1 FD 7B 02 A9 F8 5F 03 A9 F6 57 04 A9 F4 4F 05 A9 FD 83 00 91 57 D0 3B D5 F3 03 00 AA 6A 74 91 52 E8 16 40 F9 CA 45 B7 72 F5 03 02 AA F6 03 01 AA A8 83 1F F8 14 24 42 A9 E0 07 00 F9",
     0x0FBACA5C},
    {"_addCreativeItemEntry",
     "FD 7B BE A9 F4 4F 01 A9 FD 03 00 91 ?? ?? ?? ?? F4 03 00 AA 20 A0 00 91 F3 03 01 AA ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 81 62 40 39 3F 00 20 6B ?? ?? ?? ?? 68 1A 40 F9 00 01 40 F9 ?? ?? ?? ??",
     0x0FBAA994},
    {"updateNetIdMap",
     "FF C3 01 D1 FD 7B 02 A9 F9 1B 00 F9 F8 5F 04 A9 F6 57 05 A9 F4 4F 06 A9 FD 83 00 91 56 D0 3B D5 F3 03 00 AA C8 16 40 F9 A8 83 1F F8 08 44 40 F9 ?? ?? ?? ?? 74 42 40 F9 ?? ?? ?? ?? E0 03 14 AA",
     0x0FBACD10},
    {"ItemInstance name ctor",
     "FD 7B BE A9 F3 0B 00 F9 FD 03 00 91 F3 03 00 AA ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 68 02 00 F9 F3 0B 40 F9 FD 7B C2 A8 C0 03 5F D6 FF 43 03 D1 FD 7B 08 A9 FA 67 09 A9 F8 5F 0A A9 F6 57 0B A9",
     0x0FF9BC24},
    {"ItemInstance dtor",
     "FD 7B BD A9 F5 0B 00 F9 F4 4F 02 A9 FD 03 00 91 F3 03 00 AA 00 3C 40 F9 ?? ?? ?? ?? ?? ?? ?? ?? 7F 3E 00 F9 68 02 00 F9 ?? ?? ?? ?? 08 00 40 F9 08 05 40 F9 00 01 3F D6 60 2A 40 F9 ?? ?? ?? ??",
     0x0FF7FF54},
    {"ItemStackBase::getItem",
     "08 04 40 F9 ?? ?? ?? ?? 00 01 40 F9 C0 03 5F D6 E0 03 1F AA C0 03 5F D6 FD 7B BD A9 F5 0B 00 F9",
     0x0FF7FF3C},
};

std::uint64_t fnv1a(std::string_view v) {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (unsigned char c : v) h = static_cast<std::uint64_t>(c) ^ (0x100000001b3ULL * h);
    return h;
}

} // namespace

HashedStringExact::HashedStringExact(std::string value)
    : hash(fnv1a(value)), text(std::move(value)), lastMatch(nullptr) {}

bool EngineApi::resolve() {
    clear();
    ModuleView mod{};
    if (!findModule(mod)) {
        elog("[ERROR] libminecraftpe.so no encontrada");
        return false;
    }
    elog("[INFO] libminecraftpe base=0x%lx", (unsigned long)mod.base);

    for (auto const& t : kTargets) {
        if (!matchRva(mod, t.rva, t.signature)) {
            elog("[ERROR] Signature falló: %s @ 0x%lx", t.label, (unsigned long)t.rva);
            clear();
            return false;
        }
        elog("[INFO] OK %s @ 0x%lx", t.label, (unsigned long)(mod.base + t.rva));
    }

    addLooseCreativeItemsTarget = reinterpret_cast<void*>(mod.base + kTargets[0].rva);
    getCreativeCategory = reinterpret_cast<GetCreativeCategoryFn>(mod.base + kTargets[1].rva);
    addChildGroup = reinterpret_cast<AddChildGroupFn>(mod.base + kTargets[2].rva);
    newItemEntry = reinterpret_cast<NewItemEntryFn>(mod.base + kTargets[3].rva);
    addCreativeItemEntry = reinterpret_cast<AddCreativeItemEntryFn>(mod.base + kTargets[4].rva);
    updateNetIdMap = reinterpret_cast<UpdateNetIdMapFn>(mod.base + kTargets[5].rva);
    itemInstanceNameCtor = reinterpret_cast<ItemInstanceNameCtorFn>(mod.base + kTargets[6].rva);
    itemInstanceDtor = reinterpret_cast<ItemInstanceDtorFn>(mod.base + kTargets[7].rva);
    itemGetItem = reinterpret_cast<ItemGetItemFn>(mod.base + kTargets[8].rva);

    elog("[INFO] EngineApi resuelto para 1.26.52.3");
    return true;
}

void EngineApi::clear() {
    addLooseCreativeItemsTarget = nullptr;
    addLooseCreativeItemsOriginal = nullptr;
    getCreativeCategory = nullptr;
    addChildGroup = nullptr;
    newItemEntry = nullptr;
    addCreativeItemEntry = nullptr;
    updateNetIdMap = nullptr;
    itemInstanceNameCtor = nullptr;
    itemInstanceDtor = nullptr;
    itemGetItem = nullptr;
}

} // namespace inventorytab
