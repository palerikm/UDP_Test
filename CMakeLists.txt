cmake_minimum_required(VERSION 3.15cmake )
project(UDP_tests CXX C)

set(CMAKE_C_STANDARD 99)

include(FetchContent)

FetchContent_Declare(
        sockaddrutil
        GIT_REPOSITORY "https://github.com/palerikm/sockaddrutil"
        # Uncomment and set the following to a specific hash to
        # lock down to a specific commit and avoid contacting
        # the remote once we've already cloned/fetched it
        #GIT_TAG someGitHash
)

FetchContent_MakeAvailable(sockaddrutil)




# Find the QtWidgets library
find_package(Qt5 QUIET COMPONENTS Core Widgets)


## include the parts
#add_subdirectory ( include )
add_subdirectory(sockethelperlib)
add_subdirectory(udpjitterlib)
add_subdirectory(cliclient)
add_subdirectory(server)

if (${Qt5_FOUND})
    MESSAGE(STATUS "Found Qt5")
    add_subdirectory( qtclient)
else (${Qt5_FOUND})
    MESSAGE(STATUS "Could not locate Qt5  - qtclient will NOT be buildt")
ENDIF (${Qt5_FOUND})