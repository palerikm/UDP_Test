
set( LIB_NAME "udpjitter")


include(FetchContent)

FetchContent_Declare(
        hashmap_c
        GIT_REPOSITORY "https://github.com/palerikm/hashmap.c"
        #GIT_TAG someGitHash
)
FetchContent_MakeAvailable(hashmap_c)




add_library( ${LIB_NAME} src/udpjitter.c include/udpjitter.h src/timing.c include/timing.h src/fivetuple.c include/fivetuple.h)
target_link_libraries( ${LIB_NAME} hashmap_c )
target_include_directories (${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PUBLIC )
include_directories ( ${hashmap_c_SOURCE_DIR} ${sockaddrutil_SOURCE_DIR}/include )

add_subdirectory(test)