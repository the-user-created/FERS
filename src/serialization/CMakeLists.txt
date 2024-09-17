set(SRCS
	xmlimport.cpp
	response_renderer.cpp
	hdf5_export.cpp
	pulse_factory.cpp
	receiver_export.cpp
)

set(HEADERS
	xmlimport.h
	response_renderer.h
	hdf5_export.h
	pulse_factory.h
	receiver_export.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
