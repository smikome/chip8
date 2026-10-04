cmake_minimum_required(VERSION 3.20)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg 経由のライブラリを探す
find_package(fmt CONFIG REQUIRED)

add_executable(chip8 src/main.cpp)

target_link_libraries(chip8 PRIVATE fmt::fmt)
