set(SRCS
	xmlimport.cpp
	response_renderer.cpp
	hdf5_handler.cpp
	pulse_factory.cpp
	receiver_export.cpp
	response.cpp
)

set(HEADERS
	xmlimport.h
	response_renderer.h
	hdf5_handler.h
	pulse_factory.h
	receiver_export.h
	response.h
	libxml_wrapper.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
