include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(${PROJECT_NAME})

add_subdirectory(core)
add_subdirectory(radar)
add_subdirectory(simulation)
add_subdirectory(serialization)
add_subdirectory(math_utils)
add_subdirectory(python)
add_subdirectory(interpolation)

find_library(FERS_LIB_HDF5 NAMES hdf5)

if (FERS_LIB_HDF5)
	target_link_libraries(${PROJECT_NAME} ${FERS_LIB_HDF5})
	add_definitions(-DHAVE_LIBHDF5)
endif ()

find_library(FERS_LIB_HDF5_HL NAMES hdf5_hl)

if (FERS_LIB_HDF5_HL)
	target_link_libraries(${PROJECT_NAME} ${FERS_LIB_HDF5_HL})
endif ()

find_library(FERS_LIB_TINYXML NAMES tinyxml)

if (FERS_LIB_TINYXML)
	target_link_libraries(${PROJECT_NAME} ${FERS_LIB_TINYXML})
endif ()

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

if (Python3_FOUND)
	target_link_libraries(${PROJECT_NAME} Python3::Python)
	include_directories(${Python3_INCLUDE_DIRS})
endif ()

if (WIN32)
	set(Boost_USE_STATIC_LIBS ON)
else ()
	set(Boost_USE_STATIC_LIBS OFF)
endif ()

find_package(Boost COMPONENTS system thread REQUIRED)

if (Boost_LIBRARIES)
	target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif ()

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
