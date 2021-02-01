cmake_minimum_required(VERSION 3.15cmake )
project(UDP_tests CXX C)

set(CMAKE_C_STANDARD 99)

enable_testing()

include(FetchContent)

FetchContent_Declare(
        sockaddrutil
        GIT_REPOSITORY "https://github.com/palerikm/sockaddrutil"
        #GIT_TAG someGitHash
)
FetchContent_Declare(
        clogger
        GIT_REPOSITORY "https://github.com/palerikm/c-logger"
        #GIT_TAG someGitHash
)

FetchContent_MakeAvailable(sockaddrutil clogger )




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