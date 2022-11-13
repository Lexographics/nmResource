cmake_minimum_required(VERSION 3.0.0)
project(nmResource VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(nmres src/main.cpp)
set_property(TARGET nmres PROPERTY CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE MATCHES Release)
   target_compile_options(nmres PRIVATE -O2)
else() # Debug
   target_compile_options(nmres PRIVATE -O0 -g)
endif()

install(TARGETS nmres
        RUNTIME DESTINATION ./)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
