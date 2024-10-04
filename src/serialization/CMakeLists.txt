set(SRCS
	response_renderer.cpp
	hdf5_handler.cpp
	pulse_factory.cpp
	receiver_export.cpp
	response.cpp
	xml_parser.cpp
)

set(HEADERS
	response_renderer.h
	hdf5_handler.h
	pulse_factory.h
	receiver_export.h
	response.h
	libxml_wrapper.h
	xml_parser.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
