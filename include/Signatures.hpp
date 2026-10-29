#pragma once
#include <cstdint>
#include <string_view>

namespace inventorytab {

enum class SignatureId : std::size_t {
    ContainerScreenControllerOpen = 0,
    ContainerScreenControllerDtor,
    ContainerScreenControllerOnContainerSlotSelected,
    ContainerScreenControllerGetItemStack,
    ContainerScreenControllerHandleSelectSlot,
    ContainerScreenControllerGetSelectedSlotInfo,
    ScreenViewRender,
    Count
};

bool resolveAll(std::string_view libraryName = "libminecraftpe.so");
std::uintptr_t resolve(SignatureId id);
void clear();

}
