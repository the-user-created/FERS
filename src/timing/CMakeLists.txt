set(SRCS
	prototype_timing.cpp
	timing.cpp
)

set(HEADERS
	prototype_timing.h
	timing.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
