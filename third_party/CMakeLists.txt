
# gtest and gmock, only to be added explicitely (by the test target)
# set(gtest_disable_pthreads ON)
add_subdirectory(googletest EXCLUDE_FROM_ALL)
include(GoogleTest)
