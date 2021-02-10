
file(GLOB src "src/*.cpp")

add_library(imgui ${src})

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CMAKE_BUILD_TYPE RELEASE)


set_target_properties(imgui
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin"
)
