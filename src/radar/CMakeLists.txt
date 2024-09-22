set(SRCS
	radar_system.cpp
	target.cpp
	platform.cpp
)


set(HEADERS
	radar_system.h
	platform.h
	target.h
	object.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
