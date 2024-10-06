set(SRCS
	radar_signal.cpp
	dsp_filters.cpp
)


set(HEADERS
	jones_vector.h
	radar_signal.h
	dsp_filters.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
