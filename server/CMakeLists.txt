
add_executable ( udpserver udpserver.c )

target_link_libraries ( udpserver LINK_PUBLIC
        udpjitter
        sockaddrutil
        sockethelper
        pthread )

include_directories ( ../include ${sockaddrutil_SOURCE_DIR}/include)
link_directories ( ${CHECK_LIBRARY_DIRS} )