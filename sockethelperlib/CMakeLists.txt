add_library( sockethelper src/sockethelper.c include/sockethelper.h
             src/iphelper.c include/iphelper.h)



target_include_directories (sockethelper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories ( ${sockaddrutil_SOURCE_DIR}/include)