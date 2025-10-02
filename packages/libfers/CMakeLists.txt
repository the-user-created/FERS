project(libfers)

# Create the library target
add_library(libfers STATIC)
set_target_properties(libfers PROPERTIES OUTPUT_NAME "fers")

# Apply project-wide compiler warnings and flags
apply_fers_warnings(libfers)

# --- Source and Header Files ---
include(include/libfers/CMakeLists.txt)
include(src/CMakeLists.txt)

target_sources(libfers PRIVATE
	${LIBFERS_SOURCES}
	${LIBFERS_PUBLIC_HEADERS}
	${LIBFERS_PRIVATE_HEADERS}
)

source_group("Source Files" FILES ${LIBFERS_SOURCES})
source_group("Header Files\\Public" FILES ${LIBFERS_PUBLIC_HEADERS})
source_group("Header Files\\Private" FILES ${LIBFERS_PRIVATE_HEADERS})

# --- Include Directories ---
target_include_directories(libfers PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
target_include_directories(libfers PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- Schema Headers ---
set(SCHEMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../schema)
set(GENERATED_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_headers)

add_custom_command(
	OUTPUT ${GENERATED_HEADERS_DIR}/fers_xml_dtd.h ${GENERATED_HEADERS_DIR}/fers_xml_xsd.h
	COMMAND ${CMAKE_COMMAND} -E make_directory ${GENERATED_HEADERS_DIR}
	COMMAND ${CMAKE_COMMAND} -E copy ${SCHEMA_DIR}/fers-xml.dtd ${CMAKE_CURRENT_BINARY_DIR}/fers-xml.dtd
	COMMAND ${CMAKE_COMMAND} -E copy ${SCHEMA_DIR}/fers-xml.xsd ${CMAKE_CURRENT_BINARY_DIR}/fers-xml.xsd
	COMMAND xxd -i fers-xml.dtd > ${GENERATED_HEADERS_DIR}/fers_xml_dtd.h
	COMMAND xxd -i fers-xml.xsd > ${GENERATED_HEADERS_DIR}/fers_xml_xsd.h
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS ${SCHEMA_DIR}/fers-xml.dtd ${SCHEMA_DIR}/fers-xml.xsd
	COMMENT "Converting XML schema files to C headers"
)
add_custom_target(generate_schemas ALL DEPENDS
	${GENERATED_HEADERS_DIR}/fers_xml_dtd.h
	${GENERATED_HEADERS_DIR}/fers_xml_xsd.h
)
add_dependencies(libfers generate_schemas)
target_include_directories(libfers PRIVATE
	$<BUILD_INTERFACE:${GENERATED_HEADERS_DIR}>
)


# --- Third-Party Dependencies ---
# Vendored dependencies
add_subdirectory(third_party/highfive EXCLUDE_FROM_ALL)
add_subdirectory(third_party/geographiclib EXCLUDE_FROM_ALL)

# System dependencies
find_package(LibXml2 REQUIRED)
find_package(HDF5 COMPONENTS C HL REQUIRED)

# Link dependencies privately. Consumers of libfers don't need to know about them.
# NOTE: Using HDF5 variables because some systems provide an older FindHDF5.cmake module
# that does not create imported targets.
target_link_libraries(libfers PRIVATE
	HighFive::HighFive
	GeographicLib::GeographicLib
	LibXml2::LibXml2
	${HDF5_C_LIBRARIES}
	${HDF5_HL_LIBRARIES}
)
target_compile_definitions(libfers PRIVATE HAVE_LIBHDF5)

# --- Installation ---
install(TARGETS libfers
	EXPORT libfers-targets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

install(FILES ${LIBFERS_PUBLIC_HEADERS}
	DESTINATION include/libfers
)

install(EXPORT libfers-targets
	FILE libfers-config.cmake
	NAMESPACE fers::
	DESTINATION lib/cmake/libfers
)
