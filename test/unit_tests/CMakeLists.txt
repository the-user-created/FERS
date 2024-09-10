include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_BINARY_DIR}
		${fers_SOURCE_DIR}/src
)

function(add_test_executable TEST_NAME)
	add_executable(${TEST_NAME} ${ARGN})
	find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
	if (Python3_FOUND)
		target_link_libraries(${TEST_NAME} Python3::Python)
		include_directories(${Python3_INCLUDE_DIRS})
	endif ()
	if (Boost_LIBRARIES)
		target_link_libraries(${TEST_NAME} ${Boost_LIBRARIES})
	endif ()
endfunction()

if (WIN32)
	set(Boost_USE_STATIC_LIBS ON)
else ()
	set(Boost_USE_STATIC_LIBS OFF)
endif ()

find_package(Boost COMPONENTS system thread REQUIRED)
message(STATUS "Linking libraries for Boost: ${Boost_LIBRARIES}")

set(COMMON_SRCS
	${fers_SOURCE_DIR}/src/rsnoise.cpp
	${fers_SOURCE_DIR}/src/rsnoise.h
	${fers_SOURCE_DIR}/src/rsdsp.cpp
	${fers_SOURCE_DIR}/src/rsdsp.h
	${fers_SOURCE_DIR}/src/rspython.cpp
	${fers_SOURCE_DIR}/src/rspython.h
	${fers_SOURCE_DIR}/src/rsparameters.cpp
	${fers_SOURCE_DIR}/src/rsparameters.h
	${fers_SOURCE_DIR}/src/rsgeometry.cpp
	${fers_SOURCE_DIR}/src/rsgeometry.h
	${fers_SOURCE_DIR}/src/rsdebug.cpp
	${fers_SOURCE_DIR}/src/rsdebug.h
)

add_test_executable(noisegen_test noisegen_test.cpp ${COMMON_SRCS})
add_test_executable(timing_test timing_test.cpp ${COMMON_SRCS} ${fers_SOURCE_DIR}/src/rstiming.cpp ${fers_SOURCE_DIR}/src/rstiming.h)
add_test_executable(clock_test clock_test.cpp ${COMMON_SRCS} ${fers_SOURCE_DIR}/src/rstiming.cpp ${fers_SOURCE_DIR}/src/rstiming.h)
add_test_executable(filter_test filter_test.cpp ${COMMON_SRCS})
