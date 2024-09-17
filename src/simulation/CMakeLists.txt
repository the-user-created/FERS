set(SRCS
	sim_threading.cpp
	response.cpp
	noise_generators.cpp
	timing.cpp
	falpha_branch.cpp
	noise_utils.cpp
)

set(HEADERS
	sim_threading.h
	response.h
	noise_generators.h
	timing.h
	falpha_branch.h
	noise_utils.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
