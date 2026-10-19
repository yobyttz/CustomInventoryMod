cmake_minimum_required(VERSION 3.21)
project(CustomInventoryMod)

set(CMAKE_CXX_STANDARD 20)

find_package(LeviLamina REQUIRED)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    LeviLamina::LeviLamina
)
