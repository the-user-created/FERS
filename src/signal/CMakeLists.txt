set(SRCS
	radar_signal.cpp
	dsp_filters.cpp
)


set(HEADERS
	radar_signal.h
	dsp_filters.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
