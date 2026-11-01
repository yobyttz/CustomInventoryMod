cmake_minimum_required(VERSION 3.22)

project(inventorytab LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MOD_LIBRARY_NAME inventorytab)

include(FetchContent)

FetchContent_Declare(
    preloader_android
    GIT_REPOSITORY https://github.com/LiteLDev/preloader-android.git
    GIT_TAG        0.2.2
)

set(PRELOADER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(preloader_android)

set(PRELOADER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(preloader_android)

add_library(${MOD_LIBRARY_NAME} SHARED
    src/main.cpp
    src/InventoryTab.cpp
    src/EngineApi.cpp
    src/CreativeTab.cpp
)

target_include_directories(${MOD_LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${MOD_LIBRARY_NAME} PRIVATE
    preloader
    log
)

set_target_properties(${MOD_LIBRARY_NAME} PROPERTIES
    OUTPUT_NAME "inventorytab"
    PREFIX "lib"
)

if(ANDROID)
    target_compile_options(${MOD_LIBRARY_NAME} PRIVATE -fvisibility=hidden)
endif()
