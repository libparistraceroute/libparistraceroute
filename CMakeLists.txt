cmake_minimum_required(VERSION 3.2)
project(paris_traceroute_from_makefile)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -fpermissive")
set(CMAKE_VERBOSE_MAKEFILE on)
message("CMAKE CURRENT BINARY DIR: " + ${CMAKE_CURRENT_BINARY_DIR})
message("CMAKE CURRENT SOURCE DIR: " + ${CMAKE_CURRENT_SOURCE_DIR})

include(ExternalProject)
ExternalProject_Add(project_libparistraceroute
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libparistraceroute
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libparistraceroute
        CONFIGURE_COMMAND ../../../../configure
        BUILD_COMMAND make
        BUILD_ALWAYS 1
        INSTALL_COMMAND make install
        )

ExternalProject_Get_Property(project_libparistraceroute install_dir)
message("INSTALL DIR: " + ${install_dir})
add_library(libparistraceroute SHARED IMPORTED)

set_property(TARGET libparistraceroute PROPERTY IMPORTED_LOCATION ${install_dir}/src/project_libparistraceroute-build/libparistraceroute/.libs/libparistraceroute-0.9.so)

add_dependencies(libparistraceroute project_libparistraceroute)

add_executable(paris-traceroute paris-traceroute/paris-traceroute.cpp paris-traceroute/StdCapture.cpp paris-traceroute/StdCapture.hpp paris-traceroute/OutputParser.cpp paris-traceroute/OutputParser.hpp paris-traceroute/PacketWrapper.cpp paris-traceroute/PacketWrapper.hpp)

include_directories(/home/osboxes/libparistraceroute/libparistraceroute)
#set(libparistraceroutelib ${install_dir}/src/project_libparistraceroute-build/libparistraceroute/.libs/libparistraceroute-0.9.so)
# Find libtins library
FIND_LIBRARY(TINS_LIBRARY tins)
IF(TINS_LIBRARY)
    MESSAGE(STATUS "Tins library found in ${TINS_LIBRARY}")
ELSE()
    MESSAGE(FATAL_ERROR "Tins library not found.")
ENDIF()

target_link_libraries(paris-traceroute libparistraceroute tins)
