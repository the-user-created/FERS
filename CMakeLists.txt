cmake_minimum_required(VERSION 3.13)
project(fers)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Useful for static code analysis tools

# Enforce C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add the directory where config.h is located to the include path
include_directories(${CMAKE_SOURCE_DIR})

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type specified, defaulting to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "Setting compiler flags for GNU C++ compiler")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -ffast-math -fno-finite-math-only")

	if (CMAKE_BUILD_TYPE MATCHES "Release")
		message(STATUS "Setting compiler flags for Release")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
	endif ()

	if (CMAKE_BUILD_TYPE MATCHES "Debug")
		message(STATUS "Setting compiler flags for Debug")
		# Include the -g flag for debugging information
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
	endif ()
endif ()

add_definitions(-D_REENTRANT)

add_subdirectory(src)
add_subdirectory(third_party/HighFive)

target_link_libraries(${PROJECT_NAME} HighFive)
