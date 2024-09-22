set(SRCS
	antenna_factory.cpp
	antenna_pattern.cpp
)


set(HEADERS
	antenna_factory.h
	antenna_pattern.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
