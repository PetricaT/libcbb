cmake_minimum_required(VERSION 3.16)
project(libcbb VERSION 0.1.0 LANGUAGES C)

# libcbb is a stb-style amalgamated single-header library. It has no compiled
# translation units of its own - the consumer compiles the implementation
# inline by defining CBB_IMPLEMENTATION in exactly one .c file before
# including the header. This CMakeLists exposes the header path via an
# INTERFACE library so consumers can `target_link_libraries(... libcbb)`
# and pick up the include directory, and it builds the test executable
# (the single TU that defines CBB_IMPLEMENTATION).

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Header-only interface library.
add_library(libcbb INTERFACE)
target_include_directories(libcbb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(libcbb INTERFACE c_std_11)
target_compile_options(libcbb INTERFACE -Wall -Wextra -Wpedantic)

# Compatibility shim. The legacy src/libcbb.c is kept for existing builds
# but is NOT compiled into libcbb - it's a consumer-side convenience only.
# If LIBCBB_BUILD_COMPAT_SRC is ON we expose it as a separate static target
# so consumers wiring up the old layout still work.
option(LIBCBB_BUILD_COMPAT_SRC "Build src/libcbb.c as a static compat library" OFF)
if(LIBCBB_BUILD_COMPAT_SRC)
    add_library(libcbb_compat STATIC src/libcbb.c)
    target_link_libraries(libcbb_compat PUBLIC libcbb)
    target_compile_options(libcbb_compat PRIVATE -Wall -Wextra -Wpedantic)
    set_target_properties(libcbb_compat PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

option(LIBCBB_BUILD_TESTS "Build libcbb tests" ON)
if(LIBCBB_BUILD_TESTS)
    enable_testing()
    add_executable(test_libcbb tests/test_libcbb.c)
    target_link_libraries(test_libcbb PRIVATE libcbb)
    add_test(NAME test_libcbb COMMAND test_libcbb)
endif()

include(GNUInstallDirs)
install(FILES include/libcbb.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if(LIBCBB_BUILD_COMPAT_SRC)
    install(TARGETS libcbb_compat ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()