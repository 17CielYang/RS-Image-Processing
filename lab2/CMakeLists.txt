cmake_minimum_required(VERSION 3.14)
project(MyGDALProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Platform-specific configuration
if (WIN32)
    # Specify compiler (usually set in a toolchain file, not directly in CMakeLists.txt)
    set(CMAKE_C_COMPILER "gcc")
    set(CMAKE_CXX_COMPILER "g++")

    # Find GDAL library
    find_package(GDAL CONFIG)
    if(NOT GDAL_FOUND)
        find_package(GDAL REQUIRED)
    endif()
else()
    # For non-Windows platforms (e.g., Linux or MacOS), set the GDAL library search path
    set(GDAL_DIR "/opt/anaconda3/envs/GDAL")
    find_package(GDAL REQUIRED PATHS ${GDAL_DIR}/lib/cmake/GDAL)
endif()

# Include GDAL headers
include_directories(${GDAL_INCLUDE_DIRS})

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/Debug)

# Add executable including all necessary source files
add_executable(lab2 lab2.cpp)

# Link GDAL library
target_link_libraries(lab2 PRIVATE GDAL::GDAL)