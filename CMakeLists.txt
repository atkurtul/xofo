cmake_minimum_required(VERSION 3.16)

project(xofo CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-pthread -std=c++2a")

if (CMAKE_BUILD_TYPE EQUAL "")
 set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(ASSIMP REQUIRED)
find_package(glfw3 REQUIRED)
find_package(mango REQUIRED)

file(GLOB imgui_src ${CMAKE_CURRENT_SOURCE_DIR}/lib/imgui/*.cpp)
add_library(imgui ${imgui_src})

add_library(vma ${CMAKE_CURRENT_SOURCE_DIR}/lib/vma.cpp)

add_library(spirv-reflect ${CMAKE_CURRENT_SOURCE_DIR}/lib/spirv_reflect.c)


file(GLOB xofo_src ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(xofo ${xofo_src})

target_link_directories(xofo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)


target_link_libraries(xofo ${Vulkan_LIBRARIES})
target_link_libraries(xofo ${mango_LIBRARIES})
target_link_libraries(xofo ${ASSIMP_LIBRARIES})
target_link_libraries(xofo glfw)
target_link_libraries(xofo imgui)
target_link_libraries(xofo spirv-reflect)
target_link_libraries(xofo vma)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/spirv-reflect)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${mango_INCLUDE_DIRS})
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${assimp_INCLUDE_DIRS})
