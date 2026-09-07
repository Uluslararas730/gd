cmake_minimum_required(VERSION 3.21)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to the geode SDK!")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

project(GDSaveTool VERSION 1.0.0)

file(GLOB SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# zlib is used for raw deflate/inflate (gzip-style) and CRC32, matching the
# original Python implementation. On Android this links against the
# system libz.so which is always present, so no extra bundling is needed.
find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
