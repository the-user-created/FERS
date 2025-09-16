cmake_minimum_required(VERSION 3.13)
project(fers)

if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# On macOS, when using a Homebrew LLVM toolchain for modern C++ features,
	# we must explicitly tell the linker where to find the corresponding
	# Homebrew libc++ library. Otherwise, it defaults to the older,
	# incompatible system version.
	message(STATUS "macOS with Clang detected. Setting linker paths for Homebrew libc++.")
	link_directories(/usr/local/opt/llvm/lib/c++)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Useful for static code analysis tools

# Enforce C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Schema files
set(SCHEMA_DIR ${CMAKE_SOURCE_DIR}/../schema)
set(SCHEMA_HEADERS_DIR ${CMAKE_BINARY_DIR}/generated)

# Ensure that the generated headers are included in the include path
include_directories(${SCHEMA_HEADERS_DIR})

if (NOT CMAKE_BUILD_TYPE)
	# You can set the build type you would like to use by passing the -DCMAKE_BUILD_TYPE flag to cmake, e.g.,
	# 	-D CMAKE_BUILD_TYPE=Debug
	# Note: Because the Release build type is using -O2 optimization, some accuracy is lost
	# 		which will affect the results (the regression tests in test/sim_tests/ will fail).
	# 		Therefore, when using run_sim_tests.py, make sure to use the Release build type.
	message(STATUS "No build type specified, defaulting to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif ()

if (CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "Setting compiler flags for GNU C++ compiler")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -ffast-math -fno-finite-math-only -D_REENTRANT")

	if (CMAKE_BUILD_TYPE MATCHES "Release")
		message(STATUS "Setting compiler flags for Release")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
	endif ()

	if (CMAKE_BUILD_TYPE MATCHES "Debug")
		message(STATUS "Setting compiler flags for Debug")
		# Include the -g flag for debugging information
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wunused -Wall -Wextra -fprofile-arcs -ftest-coverage")
	endif ()
endif ()

# Add subdirectories for source and third-party libraries
add_subdirectory(src)
add_subdirectory(third_party/highfive)
add_subdirectory(third_party/geographiclib)

# Ensure the output directory for generated headers exists
file(MAKE_DIRECTORY ${SCHEMA_HEADERS_DIR})

# DTD and XSD files to be embedded
set(SCHEMA_FILES
	${SCHEMA_DIR}/fers-xml.dtd
	${SCHEMA_DIR}/fers-xml.xsd
)

# Define the generated headers
set(GENERATED_HEADERS
	${SCHEMA_HEADERS_DIR}/fers_xml_dtd.h
	${SCHEMA_HEADERS_DIR}/fers_xml_xsd.h
)

# Command to convert schema files to C arrays using xxd
add_custom_command(
		OUTPUT ${SCHEMA_HEADERS_DIR}/fers_xml_dtd.h ${SCHEMA_HEADERS_DIR}/fers_xml_xsd.h
		COMMAND xxd -i fers-xml.dtd > ${SCHEMA_HEADERS_DIR}/fers_xml_dtd.h
		COMMAND xxd -i fers-xml.xsd > ${SCHEMA_HEADERS_DIR}/fers_xml_xsd.h
		WORKING_DIRECTORY ${SCHEMA_DIR}
		DEPENDS ${SCHEMA_FILES}
		COMMENT "Converting XML schema files to C headers"
)

# Make sure that the generated headers are included when building your src folder
add_custom_target(generate_schemas ALL DEPENDS ${GENERATED_HEADERS})
add_dependencies(${PROJECT_NAME} generate_schemas)

target_link_libraries(${PROJECT_NAME} HighFive)
target_link_libraries(${PROJECT_NAME} GeographicLib::GeographicLib)
