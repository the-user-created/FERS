set(SRCS
	radar_obj.cpp
	target.cpp
	transmitter.cpp
	receiver.cpp
)


set(HEADERS
	radar_obj.h
	platform.h
	target.h
	object.h
	transmitter.h
	receiver.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
