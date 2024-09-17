set(SRCS
	noise_generators.cpp
	noise_utils.cpp
	falpha_branch.cpp
)

set(HEADERS
	noise_generators.h
	noise_utils.h
	falpha_branch.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
