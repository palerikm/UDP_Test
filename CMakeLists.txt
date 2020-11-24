cmake_minimum_required(VERSION 3.13cmake )
project(UDP_tests C)

set(CMAKE_C_STANDARD 99)


Include(ExternalProject)
include(FetchContent)

FetchContent_Declare(
        hashmap_c
        GIT_REPOSITORY "https://github.com/palerikm/hashmap.c"
        # Uncomment and set the following to a specific hash to
        # lock down to a specific commit and avoid contacting
        # the remote once we've already cloned/fetched it
        #GIT_TAG someGitHash
)

FetchContent_MakeAvailable(hashmap_c)
#FetchContent_GetProperties(hashmap) #mispelled name in original post
#if(NOT hashmap_POPULATED)   # name is lowercased
#    FetchContent_Populate(hashmap)
#    message(STATUS "hashmap source dir: ${hashmap_SOURCE_DIR}")
#    message(STATUS "hashmap binary dir: ${hashmap_BINARY_DIR}")
#    message(STATUS "hashmap include dir: ${hashmap_INCLUDE_DIR}")
#    add_subdirectory(${hashmap_SOURCE_DIR} ${hashmap_BINARY_DIR}) # name is lowercased
#endif()

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