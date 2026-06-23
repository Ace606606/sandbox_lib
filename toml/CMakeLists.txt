cmake_minimum_required(VERSION 3.14)
project(TomlTest LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(sandbox_toml main.cpp)

target_include_directories(sandbox_toml PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/tomlplusplus/include")

configure_file(
     "${CMAKE_CURRENT_SOURCE_DIR}/config.toml"
     "${CMAKE_BINARY_DIR}/config.toml"
     COPYONLY
)