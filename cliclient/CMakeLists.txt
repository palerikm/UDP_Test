
add_executable ( udpclient
        udpclient.c  clientcommon.c ../include/clientcommon.h )

target_link_libraries ( udpclient LINK_PUBLIC
        udpjitter
        sockaddrutil
        sockethelper
        pthread )


#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

include_directories ( ../include ${sockaddrutil_SOURCE_DIR}/include)
link_directories ( ${CHECK_LIBRARY_DIRS} )