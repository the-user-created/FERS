set(SRCS
	noise_generators.cpp
	falpha_branch.cpp
)

set(HEADERS
	noise_generators.h
	falpha_branch.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
