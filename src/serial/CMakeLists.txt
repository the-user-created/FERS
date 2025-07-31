set(SRCS
	hdf5_handler.cpp
	pulse_factory.cpp
	receiver_export.cpp
	response.cpp
	libxml_wrapper.cpp
	xml_parser.cpp
	kml_generator.cpp
)

set(HEADERS
	hdf5_handler.h
	pulse_factory.h
	receiver_export.h
	response.h
	libxml_wrapper.h
	xml_parser.h
	kml_generator.h
)

target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HEADERS})
