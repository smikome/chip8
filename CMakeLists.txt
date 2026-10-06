cmake_minimum_required(VERSION 3.20)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# vcpkg 経由のライブラリを探す
find_package(fmt CONFIG REQUIRED)
find_package(minifb CONFIG REQUIRED)

# ヘッダーオンリーライブラリ
find_path(MINIAUDIO_INCLUDE_DIR NAMES miniaudio.h REQUIRED)

add_executable(chip8 src/main.cpp)

target_link_libraries(chip8 PRIVATE
    fmt::fmt
    minifb::minifb
)

target_include_directories(chip8 PRIVATE ${MINIAUDIO_INCLUDE_DIR})
