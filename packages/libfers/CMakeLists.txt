project(libfers)

# Create the library target
add_library(libfers STATIC)

set_target_properties(libfers PROPERTIES OUTPUT_NAME "fers")

# --- Source Files ---
# Gather all .cpp files from the src directory recursively
file(GLOB_RECURSE LIBFERS_SOURCES "src/*.cpp")
target_sources(libfers PRIVATE ${LIBFERS_SOURCES})

# --- Include Directories ---
# PUBLIC: Consumers of this library will automatically get this include path.
target_include_directories(libfers PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
# PRIVATE: Only for compiling this library's internals.
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
add_subdirectory(third_party/highfive)
add_subdirectory(third_party/geographiclib)

# Link dependencies privately. Consumers of libfers don't need to know about them.
target_link_libraries(libfers PRIVATE HighFive GeographicLib::GeographicLib)

# Find libxml2
find_package(LibXml2 REQUIRED)
if (LIBXML2_FOUND)
	target_include_directories(libfers PRIVATE ${LIBXML2_INCLUDE_DIR})
	target_link_libraries(libfers PRIVATE ${LIBXML2_LIBRARIES})
endif()

# Find HDF5
set(FERS_LIB_HDF5 "$ENV{FERS_LIB_HDF5}" CACHE STRING "HDF5 library path")
set(FERS_LIB_HDF5_HL "$ENV{FERS_LIB_HDF5_HL}" CACHE STRING "HDF5 HL library path")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/include/hdf5/serial/" CACHE STRING "Additional compiler flags")

find_library(HDF5_LIBRARY NAMES hdf5 PATHS ${FERS_LIB_HDF5})
find_library(HDF5_HL_LIBRARY NAMES hdf5_hl PATHS ${FERS_LIB_HDF5_HL})

if (HDF5_LIBRARY AND HDF5_HL_LIBRARY)
	target_link_libraries(libfers PRIVATE ${HDF5_LIBRARY} ${HDF5_HL_LIBRARY})
	target_compile_definitions(libfers PRIVATE HAVE_LIBHDF5)
endif()

# --- Installation ---
# This defines where the library and its headers will be installed.
install(TARGETS libfers
	EXPORT libfers-targets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT libfers-targets
	FILE libfers-config.cmake
	NAMESPACE fers::
	DESTINATION lib/cmake/libfers
)
