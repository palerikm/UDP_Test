set (ADDITIONAL_LIBS "")

#find_package( OpenSSL )
#if( OPENSSL_FOUND )
#  include_directories( ${OPENSSL_INCLUDE_DIR} )
#  list(APPEND ADDITIONAL_LIBS ${OPENSSL_LIBRARIES})
#endif( OPENSSL_FOUND )

add_executable ( udpserver
        iphelper.c
        udpserver.c sockethelper.c )

target_link_libraries ( udpserver PRIVATE sockaddrutil
        pthread
        ${ADDITIONAL_LIBS})

set_target_properties(udpserver PROPERTIES INSTALL_RPATH "${install_dir}/lib")
set_target_properties(udpserver PROPERTIES BUILD_WITH_INSTALL_RPATH ON)

add_executable ( udpclient
        iphelper.c
        udpclient.c sockethelper.c )

target_link_libraries ( udpclient PRIVATE sockaddrutil
        pthread
        ${ADDITIONAL_LIBS})
set_target_properties(udpclient PROPERTIES INSTALL_RPATH "${install_dir}/lib")
set_target_properties(udpclient PROPERTIES BUILD_WITH_INSTALL_RPATH ON)



include_directories ( ../include )
link_directories ( ${CHECK_LIBRARY_DIRS} )