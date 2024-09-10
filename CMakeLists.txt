cmake_minimum_required(VERSION 3.10)
project(fers)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Add the directory where config.h is located to the include path
include_directories(${CMAKE_SOURCE_DIR})

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type specified, defaulting to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "Setting compiler flags for GNU C++ compiler")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -pthread -ffast-math")

	if (CMAKE_BUILD_TYPE MATCHES "Release")
		message(STATUS "Setting compiler flags for Release")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
	endif ()

	if (CMAKE_BUILD_TYPE MATCHES "Debug")
		message(STATUS "Setting compiler flags for Debug")
	endif ()
endif ()

add_definitions(-D_REENTRANT)

add_subdirectory(src)
add_subdirectory(utilities)
add_subdirectory(test)