find_package(GTest REQUIRED)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_executable(tests
        tests.cpp
)

target_link_libraries(tests PRIVATE src GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)