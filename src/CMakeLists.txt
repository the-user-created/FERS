include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE main.cpp config.h)

add_subdirectory(core)
add_subdirectory(radar)
add_subdirectory(serial)
add_subdirectory(math)
add_subdirectory(python)
add_subdirectory(interpolation)
add_subdirectory(noise)
add_subdirectory(timing)
add_subdirectory(signal)
add_subdirectory(antenna)

# Use provided environment variables for HDF5 or fallback to default
# The environment variables can be set when running the cmake command, e.g.,
# 	-D FERS_LIB_HDF5="/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5.so" \
#   -D FERS_LIB_HDF5_HL="/usr/lib/x86_64-linux-gnu/hdf5/serial/libhdf5_hl.so" \
#   -D CMAKE_CXX_FLAGS="-I/usr/include/hdf5/serial/"
set(FERS_LIB_HDF5 "$ENV{FERS_LIB_HDF5}" CACHE STRING "HDF5 library path")
set(FERS_LIB_HDF5_HL "$ENV{FERS_LIB_HDF5_HL}" CACHE STRING "HDF5 HL library path")
set(CMAKE_CXX_FLAGS "-I/usr/include/hdf5/serial/" CACHE STRING "Additional compiler flags")

# HDF5 Library
find_library(FERS_LIB_HDF5 NAMES hdf5 PATHS ${FERS_LIB_HDF5})
find_library(FERS_LIB_HDF5_HL NAMES hdf5_hl PATHS ${FERS_LIB_HDF5_HL})

if (FERS_LIB_HDF5)
	target_link_libraries(${PROJECT_NAME} ${FERS_LIB_HDF5})
	add_definitions(-DHAVE_LIBHDF5)
endif ()

if (FERS_LIB_HDF5_HL)
	target_link_libraries(${PROJECT_NAME} ${FERS_LIB_HDF5_HL})
endif ()

# Find libxml2
find_package(LibXml2 REQUIRED)
if (LIBXML2_FOUND)
	message(STATUS "Found libxml2: ${LIBXML2_INCLUDE_DIR}")
	include_directories(${LIBXML2_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${LIBXML2_LIBRARIES})
endif ()

# If you have multiple versions of Python installed, you can specify the version you want to use with the following command:
# 	-D Python_EXECUTABLE=/usr/bin/python3.x (e.g., python3.11)
set(Python3_EXECUTABLE ${Python_EXECUTABLE})

# Tested Python versions: 3.11.12, 3.10.15, 3.9.20, 3.8.20, 3.7.10
find_package(Python3 3.7...3.11.13 COMPONENTS Interpreter Development REQUIRED)

set(PYTHON_VERSION ${Python3_VERSION})
message(STATUS "Found Python3 version: ${PYTHON_VERSION}")

if (PYTHON_VERSION VERSION_GREATER_EQUAL "3.7" AND PYTHON_VERSION VERSION_LESS_EQUAL "3.11.13")
	message(STATUS "Python version ${PYTHON_VERSION} is compatible.")
else ()
	message(WARNING "Python version ${PYTHON_VERSION} is not supported. Use Python 3.7 to 3.11.13.")
endif ()

if (Python3_FOUND)
	message(STATUS "Python include directories: ${Python3_INCLUDE_DIRS}")
	message(STATUS "Python libraries: ${Python3_LIBRARIES}")

	target_link_libraries(${PROJECT_NAME} Python3::Python)
	include_directories(${Python3_INCLUDE_DIRS})
endif ()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
