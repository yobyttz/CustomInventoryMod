cmake_minimum_required(VERSION 3.21)
project(CustomInventoryMod CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Genera una librería nativa .so para Android
add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enlazar la librería de logs nativa de Android
target_link_libraries(${PROJECT_NAME} PRIVATE
    log
)

target_compile_options(${PROJECT_NAME} PRIVATE -fvisibility=hidden -O3)
