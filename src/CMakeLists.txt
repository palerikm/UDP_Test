set (ADDITIONAL_LIBS "")


add_executable ( udpserver
        iphelper.c
        udpserver.c sockethelper.c )

target_link_libraries ( udpserver PRIVATE sockaddrutil
        udpjitter
        pthread
        #hashmap_c
        ${ADDITIONAL_LIBS})

set_target_properties(udpserver PROPERTIES INSTALL_RPATH "${install_dir}/lib")
set_target_properties(udpserver PROPERTIES BUILD_WITH_INSTALL_RPATH ON)

add_executable ( udpclient
        iphelper.c
        udpclient.c sockethelper.c  clientcommon.c ../include/clientcommon.h )

target_link_libraries ( udpclient LINK_PUBLIC udpjitter sockaddrutil
        pthread
       # hashmap_c
        ${ADDITIONAL_LIBS})
#set_target_properties(udpclient PROPERTIES INSTALL_RPATH "${install_dir}/lib")
#set_target_properties(udpclient PROPERTIES BUILD_WITH_INSTALL_RPATH ON)

get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

include_directories ( ../include ${hashmap_c_SOURCE_DIR} ${sockaddrutil_SOURCE_DIR}/include)
link_directories ( ${CHECK_LIBRARY_DIRS} )