cmake_minimum_required(VERSION 3.10.2)
project("backpack")

# Añadimos los archivos fuente de nuestro mod y de la librería de hooks
add_library(backpack SHARED
    src/main.cpp
    src/And64InlineHook.cpp
)

# Indicamos dónde están los archivos de cabecera (.hpp)
target_include_directories(backpack PRIVATE src)

# Enlazamos las librerías nativas de Android necesarias para logs y memoria
find_library(log-lib log)
target_link_libraries(backpack ${log-lib} android)
