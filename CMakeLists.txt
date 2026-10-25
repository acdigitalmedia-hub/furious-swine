cmake_minimum_required(VERSION 3.12)
project(SnoutAndSiege VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(furious_swine src/furious_swine.cpp)
target_link_libraries(furious_swine PRIVATE ${CMAKE_DL_LIBS} m pthread)
