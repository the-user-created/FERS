set(SRCS
	sim_threading.cpp
	response.cpp
	timing.cpp
)

set(HEADERS
	sim_threading.h
	response.h
	timing.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
