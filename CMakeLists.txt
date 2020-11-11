cmake_minimum_required(VERSION 3.13cmake )
project(UDP_tests C)

set(CMAKE_C_STANDARD 99)


Include(ExternalProject)

ExternalProject_Add(
        project_sockaddrutil
        GIT_REPOSITORY https://github.com/NATTools/sockaddrutil.git
        CMAKE_ARGS -Doptimize=OFF -Dbuild_docs=OFF -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        INSTALL_DIR "${dist_dir}"
        UPDATE_DISCONNECTED 1
)

ExternalProject_Get_Property(project_sockaddrutil install_dir)
include_directories ( "${install_dir}/include" )
add_library(sockaddrutil STATIC IMPORTED)
set_property(TARGET sockaddrutil PROPERTY IMPORTED_LOCATION "${install_dir}/lib/${CMAKE_SHARED_MODULE_PREFIX}sockaddrutil${CMAKE_SHARED_LIBRARY_SUFFIX}")
add_dependencies(sockaddrutil project_sockaddrutil)

## include the parts
#add_subdirectory ( include )
add_subdirectory ( src )