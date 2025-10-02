project(libfers)

# Create the library target
add_library(libfers STATIC)

set_target_properties(libfers PROPERTIES OUTPUT_NAME "fers")

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
set(SCHEMA_HEADERS_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${SCHEMA_HEADERS_DIR})

add_custom_command(
	OUTPUT ${SCHEMA_HEADERS_DIR}/fers_xml_dtd.h ${SCHEMA_HEADERS_DIR}/fers_xml_xsd.h
	COMMAND ${CMAKE_COMMAND} -E copy ${SCHEMA_DIR}/fers-xml.dtd ${CMAKE_CURRENT_BINARY_DIR}/fers-xml.dtd
	COMMAND ${CMAKE_COMMAND} -E copy ${SCHEMA_DIR}/fers-xml.xsd ${CMAKE_CURRENT_BINARY_DIR}/fers-xml.xsd
	COMMAND xxd -i fers-xml.dtd > ${SCHEMA_HEADERS_DIR}/fers_xml_dtd.h
	COMMAND xxd -i fers-xml.xsd > ${SCHEMA_HEADERS_DIR}/fers_xml_xsd.h
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS ${SCHEMA_DIR}/fers-xml.dtd ${SCHEMA_DIR}/fers-xml.xsd
	COMMENT "Converting XML schema files to C headers"
)
add_custom_target(generate_schemas ALL DEPENDS
	${SCHEMA_HEADERS_DIR}/fers_xml_dtd.h
	${SCHEMA_HEADERS_DIR}/fers_xml_xsd.h
)
add_dependencies(libfers generate_schemas)
target_include_directories(libfers PRIVATE
	$<BUILD_INTERFACE:${SCHEMA_HEADERS_DIR}>
)

# --- Third-Party Dependencies ---
# The third_party directory is inside libfers, so we can use add_subdirectory directly.
add_subdirectory(third_party/highfive EXCLUDE_FROM_ALL)
add_subdirectory(third_party/geographiclib EXCLUDE_FROM_ALL)

# Link dependencies privately. Consumers of libfers don't need to know about them.
target_link_libraries(libfers PRIVATE HighFive::HighFive GeographicLib::GeographicLib)

# Find libxml2
find_package(LibXml2 REQUIRED)
if (LIBXML2_FOUND)
	target_include_directories(libfers PRIVATE ${LIBXML2_INCLUDE_DIR})
	target_link_libraries(libfers PRIVATE ${LIBXML2_LIBRARIES})
endif ()

# Find HDF5
find_package(HDF5 COMPONENTS C HL REQUIRED)
if (HDF5_FOUND)
	# Use the variables provided by FindHDF5 instead of the IMPORTED targets
	target_link_libraries(libfers PRIVATE ${HDF5_C_LIBRARIES} ${HDF5_HL_LIBRARIES}) # <-- FIX HERE
	target_compile_definitions(libfers PRIVATE HAVE_LIBHDF5)
endif ()

# --- Installation ---
# This defines where the library and its headers will be installed.
install(TARGETS libfers
	EXPORT libfers-targets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

# Install the public headers using the explicit file list.
install(FILES ${LIBFERS_PUBLIC_HEADERS}
	DESTINATION include/libfers
)

install(EXPORT libfers-targets
	FILE libfers-config.cmake
	NAMESPACE fers::
	DESTINATION lib/cmake/libfers
)
