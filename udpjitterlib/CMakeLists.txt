
include(FetchContent)

FetchContent_Declare(
        hashmap_c
        GIT_REPOSITORY "https://github.com/palerikm/hashmap.c"
        #GIT_TAG someGitHash
)
FetchContent_MakeAvailable(hashmap_c)



add_library( udpjitter src/udpjitter.c include/udpjitter.h src/timing.c include/timing.h)

target_link_libraries( udpjitter hashmap_c)

target_include_directories (udpjitter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories ( ${hashmap_c_SOURCE_DIR} ${sockaddrutil_SOURCE_DIR}/include)