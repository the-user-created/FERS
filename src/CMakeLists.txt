include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

set(fers_SRCS
	rsdebug.cpp rsmain.cpp rsplatform.cpp rsradar.cpp rstarget.cpp rsworld.cpp
	xmlimport.cpp rsgeometry.cpp rspath.cpp rssim.cpp rsthreadedsim.cpp
	rspulserender.cpp rsradarwaveform.cpp rsparameters.cpp rsobject.cpp
	rsantenna.cpp rsportable.cpp rsresponse.cpp rsnoise.cpp rssignal.cpp
	rstiming.cpp rshdf5.cpp rspython.cpp rsinterp.cpp rsdsp.cpp rsmultipath.cpp
	rspattern.cpp rspolarize.cpp rsdebug.h rspath.h rsportable.h rsradar.h
	rssim.h rsworld.h rsgeometry.h rsplatform.h rstarget.h xmlimport.h
	rsradarwaveform.h rsparameters.h rssignal.h rsthreadedsim.h rsobject.h
	rsantenna.h rsresponse.h rsnoise.h rstiming.h rshdf5.h rspython.h rsinterp.h
	rsdsp.h rsmultipath.h rspattern.h rspolarize.h
)

add_executable(fers ${fers_SRCS})

find_library(FERS_LIB_HDF5 NAMES hdf5)

if (FERS_LIB_HDF5)
	target_link_libraries(fers ${FERS_LIB_HDF5})
	add_definitions(-DHAVE_LIBHDF5)
endif ()

find_library(FERS_LIB_HDF5_HL NAMES hdf5_hl)

if (FERS_LIB_HDF5_HL)
	target_link_libraries(fers ${FERS_LIB_HDF5_HL})
endif ()

find_library(FERS_LIB_TINYXML NAMES tinyxml)

if (FERS_LIB_TINYXML)
	target_link_libraries(fers ${FERS_LIB_TINYXML})
endif ()

set(Python_ADDITIONAL_VERSIONS 2.7 2.6 2.5)
include(FindPythonLibs)

if (PYTHONLIBS_FOUND)
	target_link_libraries(fers ${PYTHON_LIBRARIES})
	include_directories(${PYTHON_INCLUDE_PATH})
endif ()

if (WIN32)
	set(Boost_USE_STATIC_LIBS ON)
else ()
	set(Boost_USE_STATIC_LIBS OFF)
endif ()

find_package(Boost COMPONENTS system thread REQUIRED)

if (Boost_LIBRARIES)
	target_link_libraries(fers ${Boost_LIBRARIES})
endif ()

install(TARGETS fers DESTINATION bin)
