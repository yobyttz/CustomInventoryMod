cmake_minimum_required(VERSION 3.22.1)
project(backpack)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compila la librería compartida usando tu main y los archivos de subhook
add_library(backpack SHARED 
    src/main.cpp
    src/subhook.c 
)

target_include_directories(backpack PRIVATE src)
target_link_libraries(backpack PRIVATE log)
