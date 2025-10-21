cmake_minimum_required(VERSION 3.15)
project(BASIC_GAME)
message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_TOOLCHAIN_FILE "H:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")


add_executable(BASIC_GAME main.cpp)


  # this is heuristically generated, and may not be correct
  find_package(glad CONFIG REQUIRED)
  target_link_libraries(BASIC_GAME PRIVATE glad::glad)

  # this is heuristically generated, and may not be correct
  find_package(glfw3 CONFIG REQUIRED)
  target_link_libraries(BASIC_GAME PRIVATE glfw)


    find_package(glm CONFIG REQUIRED)
  target_link_libraries(BASIC_GAME PRIVATE glm::glm)

  find_package(assimp CONFIG REQUIRED)
  target_link_libraries(BASIC_GAME PRIVATE assimp::assimp)