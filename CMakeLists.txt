# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(TankDuel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)

add_executable(tank_duel src/main.cpp)
target_link_libraries(tank_duel PRIVATE SDL2::SDL2 SDL2::SDL2main)
