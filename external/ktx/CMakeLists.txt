
file(GLOB src "lib/*.cxx" "lib/*.c")


add_library(ktx ${src})

target_compile_definitions(ktx PUBLIC -DKTX_OPENGL)
target_include_directories(ktx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

#set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)


set_target_properties(ktx
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin"
)
