set(SRCS
	logging.cpp
	main.cpp
	parameters.cpp
	world.cpp
)

set(HEADERS
	logging.h
	portable_utils.h
	parameters.h
	world.h
	object.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
