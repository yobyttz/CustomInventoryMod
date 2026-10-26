cmake_minimum_required(VERSION 3.10.2)
project("CustomInventoryMod")

# Definimos la librería compartida (.so) con sus fuentes
add_library(backpack SHARED
    src/main.cpp
    src/And64InlineHook.cpp
)

# Incluimos la carpeta src para encontrar las cabeceras (.hpp)
target_include_directories(backpack PRIVATE src)

# Enlazamos las librerías nativas de Android
find_library(log-lib log)
target_link_libraries(backpack ${log-lib} android)
