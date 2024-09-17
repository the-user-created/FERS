set(SRCS
	dsp_filters.cpp
	geometry_ops.cpp
	multipath_surface.cpp
	polarization_matrix.cpp
	rotation_path.cpp
	path.cpp
)

set(HEADERS
	dsp_filters.h
	geometry_ops.h
	multipath_surface.h
	polarization_matrix.h
	rotation_path.h
	path.h
	coord.h
	path_utils.h
)


target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
