set(SRCS
	logging.cpp
	main.cpp
	parameters.cpp
	world.cpp
	arg_parser.cpp
)

set(HEADERS
	logging.h
	portable_utils.h
	parameters.h
	world.h
	object.h
	arg_parser.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
