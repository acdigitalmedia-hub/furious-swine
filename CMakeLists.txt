cmake_minimum_required(VERSION 3.12)
project(SnoutAndSiege VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SNOUT_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(furious_swine ${SNOUT_SOURCES})
target_compile_options(furious_swine PRIVATE -Wall -Wextra -Wno-unused-parameter)
target_link_libraries(furious_swine PRIVATE ${CMAKE_DL_LIBS} m pthread)
