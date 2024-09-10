include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${HDF5_INCLUDE_DIRS})

# csv2antenna target
add_executable(csv2antenna csv2antenna.c)

# cluttergen target
add_executable(cluttergen cluttergen.cpp)

# cluttergen2d target
add_executable(cluttergen2d cluttergen2d.cpp)

# h52raw target
add_executable(h52raw h52raw.cpp)

# Link to the HDF5 libraries
find_library(FERS_LIB_HDF5 NAMES hdf5)
if (FERS_LIB_HDF5)
	target_link_libraries(h52raw ${FERS_LIB_HDF5})
endif ()

find_library(FERS_LIB_HDF5_HL NAMES hdf5_hl)
if (FERS_LIB_HDF5_HL)
	target_link_libraries(h52raw ${FERS_LIB_HDF5_HL})
endif ()
