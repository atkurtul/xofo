cmake_minimum_required(VERSION 3.16)

project(abc)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-pthread -g")
file(GLOB src src/*.cpp)
file(GLOB headers include/*.h)


add_subdirectory(external)
add_executable(vk ${headers} ${src})

set(CMAKE_BUILD_TYPE DEBUG)
#set(CMAKE_BUILD_TYPE RELEASE)


add_dependencies(vk mango_download)

target_include_directories(vk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(vk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/include)
target_include_directories(vk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/ktx/include)
target_link_directories(vk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/lib)


target_link_libraries(vk mango)
target_link_libraries(vk imgui)
target_link_libraries(vk ktx)

target_link_libraries(vk vulkan)
target_link_libraries(vk shaderc_shared)
target_link_libraries(vk spirv-cross-core)
target_link_libraries(vk assimp)
target_link_libraries(vk glfw)
