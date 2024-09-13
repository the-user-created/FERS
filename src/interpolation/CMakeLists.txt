set(SRCS
	interpolation_filter.cpp
	interpolation_set.cpp
)

set(HEADERS
	interpolation_filter.h
	interpolation_set.h
	interpolation_point.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
