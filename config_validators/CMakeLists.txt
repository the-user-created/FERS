cmake_minimum_required(VERSION 3.22)
project(kml_visualiser)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

# Add include directories for FERS source files
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})

add_executable(kml_visualiser
			   kml_visualiser.cpp
			   ../src/serial/libxml_wrapper.cpp
			   ../src/core/logging.cpp
)

target_link_libraries(kml_visualiser PRIVATE ${LIBXML2_LIBRARIES})

install(TARGETS kml_visualiser DESTINATION bin)