add_subdirectory(imgui)
add_subdirectory(ktx)


find_library(MANGO_LIB mango)
if(NOT MANGO_LIB)
  message(WARNING "mango not found building from git")
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
  
else()
add_custom_target(mango_download)
endif()

