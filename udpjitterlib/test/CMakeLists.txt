set(TEST_APP_NAME "${LIB_NAME}_test")

FetchContent_Declare(
        doctest
        GIT_REPOSITORY "https://github.com/onqtam/doctest"
        #GIT_TAG someGitHash
)
FetchContent_MakeAvailable(doctest)

add_executable(${TEST_APP_NAME} ${TEST_SOURCES} main.cpp fivetuple_test.cpp)
add_dependencies(${TEST_APP_NAME} ${LIB_NAME} )
target_include_directories(${TEST_APP_NAME} PUBLIC ${doctest_SOURCE_DIR}/doctest../include)
target_link_libraries(${TEST_APP_NAME} ${LIB_NAME}
        sockaddrutil
        doctest::doctest )

add_test(NAME ${TEST_APP_NAME} COMMAND $<TARGET_FILE:${TEST_APP_NAME}>)

# Enable CTest
include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(${TEST_APP_NAME})