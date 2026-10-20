cmake_minimum_required(VERSION 3.21)
project(CustomInventoryMod CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Descarga automatizada de Dobby en el proceso de build
set(DOBBY_PATH ${CMAKE_BINARY_DIR}/dobby)
if(NOT EXISTS ${DOBBY_PATH})
    execute_process(
        COMMAND git clone --depth 1 https://github.com/jmpews/Dobby.git ${DOBBY_PATH}
    )
endif()

add_subdirectory(${DOBBY_PATH} ${CMAKE_BINARY_DIR}/dobby_build)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${DOBBY_PATH}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dobby
    log
    android
)

target_compile_options(${PROJECT_NAME} PRIVATE -fvisibility=hidden -O3)
