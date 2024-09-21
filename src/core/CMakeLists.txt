set(SRCS
	logging.cpp
	main.cpp
	parameters.cpp
	world.cpp
	arg_parser.cpp
	sim_threading.cpp
)

set(HEADERS
	logging.h
	portable_utils.h
	parameters.h
	world.h
	arg_parser.h
	sim_threading.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
