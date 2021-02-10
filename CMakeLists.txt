cmake_minimum_required(VERSION 3.16)

project(abc)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-pthread")
file(GLOB src "src/*.cpp")

add_executable(vk ${src})

include(ExternalProject)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/external)

ExternalProject_Add(mango_download
    GIT_REPOSITORY https://github.com/atkurtul/mango
    PREFIX ${EXTERNAL_INSTALL_LOCATION}
    #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/mango
    UPDATE_COMMAND ""
    INSTALL_COMMAND cmake --install . --prefix ${EXTERNAL_INSTALL_LOCATION}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DCMAKE_BUILD_TYPE=RELEASE
)

add_dependencies(vk mango_download)

target_include_directories(vk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(vk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/include)
target_link_directories(vk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/lib)


target_link_libraries(vk mango)
target_link_libraries(vk vulkan)
target_link_libraries(vk shaderc_shared)
target_link_libraries(vk spirv-cross-core)
target_link_libraries(vk assimp)
target_link_libraries(vk glfw)
