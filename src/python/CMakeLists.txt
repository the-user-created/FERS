set(SRCS
	python_extension.cpp
)

set(HEADERS
	python_extension.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
