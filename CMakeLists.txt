cmake_minimum_required(VERSION 3.22)
project(InventoryTab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(MOD_ID "inventorytab" CACHE STRING "Mod ID")
set(MOD_NAME "Custom Inventory Tab" CACHE STRING "Mod Name")
set(MOD_AUTHOR "TuNombre" CACHE STRING "Author")
set(MOD_VERSION "0.1.0" CACHE STRING "Version")
set(MOD_LIBRARY_NAME "inventorytab" CACHE STRING "Library name")

include(FetchContent)

FetchContent_Declare(
    preloader_android
    GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
    GIT_TAG 0.2.2
)
FetchContent_MakeAvailable(preloader_android)

add_library(${MOD_LIBRARY_NAME} SHARED
    src/main.cpp
    src/InventoryTab.cpp
    src/Signatures.cpp
)

target_include_directories(${MOD_LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MOD_LIBRARY_NAME} PRIVATE preloader log)

# Nombre del .so final
set_target_properties(${MOD_LIBRARY_NAME} PROPERTIES
    OUTPUT_NAME "${MOD_LIBRARY_NAME}"
    PREFIX "lib"
)
