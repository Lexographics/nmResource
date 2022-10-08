cmake_minimum_required(VERSION 3.0.0)
project(nmResource VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(nmres src/main.cpp)
set_property(TARGET nmres PROPERTY CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
