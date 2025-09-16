set(SRCS
	geometry_ops.cpp
	# polarization_matrix.cpp
	rotation_path.cpp
	path.cpp
)

set(HEADERS
	geometry_ops.h
	# polarization_matrix.h
	rotation_path.h
	path.h
	coord.h
	path_utils.h
)


target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
