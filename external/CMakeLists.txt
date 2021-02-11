add_subdirectory(imgui)

include(ExternalProject)

set(EXTERNAL_INSTALL_LOCATION ${CMAKE_CURRENT_SOURCE_DIR})

ExternalProject_Add(mango_download
    GIT_REPOSITORY https://github.com/atkurtul/mango
    PREFIX ${EXTERNAL_INSTALL_LOCATION}/mango
    #SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/mango
    UPDATE_COMMAND ""
    INSTALL_COMMAND cmake --install . --prefix ${EXTERNAL_INSTALL_LOCATION}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DCMAKE_BUILD_TYPE=RELEASE
)


# add_library(vma ${CMAKE_CURRENT_SOURCE_DIR}/vma/vma.cpp)
# target_include_directories(vma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vma)

# set_target_properties(vma
#   PROPERTIES
#   ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
#   LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib"
#   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
# )
