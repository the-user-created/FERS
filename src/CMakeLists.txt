include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

set(fers_SRCS logging.cpp main.cpp platform.cpp radar_system.cpp target.cpp world.cpp xmlimport.cpp geometry_ops.cpp
	rspath.cpp sim_threading.cpp rspulserender.cpp radar_signal.cpp parameters.cpp antenna_factory.cpp response.cpp
	noise_generators.cpp rssignal.cpp timing.cpp hdf5_export.cpp python_extension.cpp interpolation_set.cpp
	dsp_filters.cpp multipath_surface.cpp antenna_pattern.cpp logging.h rspath.h portable_utils.h radar_system.h
	world.h geometry_ops.h platform.h target.h xmlimport.h radar_signal.h parameters.h rssignal.h sim_threading.h
	object.h antenna_factory.h response.h noise_generators.h timing.h hdf5_export.h python_extension.h
	interpolation_set.h dsp_filters.h multipath_surface.h antenna_pattern.h jones_vector.h polarization_matrix.cpp
	polarization_matrix.h interpolation_filter.cpp interpolation_filter.h
	interpolation_point.h
)

add_executable(fers ${fers_SRCS})

find_library(FERS_LIB_HDF5 NAMES hdf5)

if (FERS_LIB_HDF5)
	target_link_libraries(fers ${FERS_LIB_HDF5})
	add_definitions(-DHAVE_LIBHDF5)
endif ()

find_library(FERS_LIB_HDF5_HL NAMES hdf5_hl)

if (FERS_LIB_HDF5_HL)
	target_link_libraries(fers ${FERS_LIB_HDF5_HL})
endif ()

find_library(FERS_LIB_TINYXML NAMES tinyxml)

if (FERS_LIB_TINYXML)
	target_link_libraries(fers ${FERS_LIB_TINYXML})
endif ()

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

if (Python3_FOUND)
	target_link_libraries(fers Python3::Python)
	include_directories(${Python3_INCLUDE_DIRS})
endif ()

if (WIN32)
	set(Boost_USE_STATIC_LIBS ON)
else ()
	set(Boost_USE_STATIC_LIBS OFF)
endif ()

find_package(Boost COMPONENTS system thread REQUIRED)

if (Boost_LIBRARIES)
	target_link_libraries(fers ${Boost_LIBRARIES})
endif ()

install(TARGETS fers DESTINATION bin)
