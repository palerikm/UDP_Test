set(TEST_APP_NAME "${LIB_NAME}_test")



FetchContent_Declare(
        doctest
        GIT_REPOSITORY "https://github.com/onqtam/doctest"
        #GIT_TAG someGitHash
)
FetchContent_MakeAvailable(doctest)


# Test Binary
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${TEST_APP_NAME} ${TEST_SOURCES})
add_dependencies(${TEST_APP_NAME} ${LIB_NAME} )
target_include_directories(${TEST_APP_NAME} PUBLIC ${doctest_SOURCE_DIR}/doctest)
target_link_libraries(${TEST_APP_NAME} ${LIB_NAME}
        doctest::doctest )

add_test(NAME ${TEST_APP_NAME} COMMAND $<TARGET_FILE:${TEST_APP_NAME}>)

# Enable CTest
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(${TEST_APP_NAME})