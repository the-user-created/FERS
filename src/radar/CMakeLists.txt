set(SRCS
	antenna_factory.cpp
	antenna_pattern.cpp
	radar_signal.cpp
	radar_system.cpp
	target.cpp
	platform.cpp
)


set(HEADERS
	radar_system.h
	platform.h
	target.h
	antenna_factory.h
	antenna_pattern.h
	jones_vector.h
	radar_signal.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
