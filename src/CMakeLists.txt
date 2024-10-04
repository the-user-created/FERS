include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME})

add_subdirectory(core)
add_subdirectory(radar)
add_subdirectory(serialization)
add_subdirectory(math_utils)
add_subdirectory(python)
add_subdirectory(interpolation)
add_subdirectory(noise)
add_subdirectory(timing)
add_subdirectory(signal_processing)
add_subdirectory(antenna)

find_library(FERS_LIB_HDF5 NAMES hdf5)

if (FERS_LIB_HDF5)
	target_link_libraries(${PROJECT_NAME} ${FERS_LIB_HDF5})
	add_definitions(-DHAVE_LIBHDF5)
endif ()

find_library(FERS_LIB_HDF5_HL NAMES hdf5_hl)

if (FERS_LIB_HDF5_HL)
	target_link_libraries(${PROJECT_NAME} ${FERS_LIB_HDF5_HL})
endif ()

find_package(LibXml2 REQUIRED)
if (LIBXML2_FOUND)
	message(STATUS "Found libxml2: ${LIBXML2_INCLUDE_DIR}")
	include_directories(${LIBXML2_INCLUDE_DIR})
	target_link_libraries(${PROJECT_NAME} ${LIBXML2_LIBRARIES})
endif ()

set(Python3_EXECUTABLE ${Python_EXECUTABLE})

# Tested Python versions: 3.11.10, 3.10.15, 3.9.20, 3.8.20, 3.7.10
find_package(Python3 3.7...3.11.10 COMPONENTS Interpreter Development REQUIRED)

set(PYTHON_VERSION ${Python3_VERSION})
message(STATUS "Found Python3 version: ${PYTHON_VERSION}")

if (PYTHON_VERSION VERSION_GREATER_EQUAL "3.7" AND PYTHON_VERSION VERSION_LESS_EQUAL "3.11.10")
	message(STATUS "Python version ${PYTHON_VERSION} is compatible.")
else ()
	message(WARNING "Python version ${PYTHON_VERSION} is not supported. Use Python 3.7 to 3.11.10.")
endif ()

if (Python3_FOUND)
	message(STATUS "Python include directories: ${Python3_INCLUDE_DIRS}")
	message(STATUS "Python libraries: ${Python3_LIBRARIES}")

	target_link_libraries(${PROJECT_NAME} Python3::Python)
	include_directories(${Python3_INCLUDE_DIRS})
endif ()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
