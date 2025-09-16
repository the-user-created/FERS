set(SRCS
	antenna_factory.cpp
)


set(HEADERS
	antenna_factory.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
