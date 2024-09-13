set(SRCS
	sim_threading.cpp
	response.cpp
	noise_generators.cpp
	timing.cpp
)

set(HEADERS
	sim_threading.h
	response.h
	noise_generators.h
	timing.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
