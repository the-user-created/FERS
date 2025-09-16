set(SRCS
	logging.cpp
	world.cpp
	arg_parser.cpp
	sim_threading.cpp
	thread_pool.cpp
)

set(HEADERS
	logging.h
	portable_utils.h
	parameters.h
	world.h
	arg_parser.h
	sim_threading.h
	thread_pool.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
