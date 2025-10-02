# This file is the central manifest for the libfers private implementation. It defines:
#  - LIBFERS_PRIVATE_HEADERS: Headers used internally by the library.
#  - LIBFERS_SOURCES: The implementation source files.

# --- Private Headers ---
set(LIBFERS_PRIVATE_HEADERS
	${CMAKE_CURRENT_LIST_DIR}/core/fers_context.h
	${CMAKE_CURRENT_LIST_DIR}/core/sim_threading.h
	${CMAKE_CURRENT_LIST_DIR}/core/thread_pool.h
	${CMAKE_CURRENT_LIST_DIR}/interpolation/interpolation_filter.h
	${CMAKE_CURRENT_LIST_DIR}/interpolation/interpolation_point.h
	${CMAKE_CURRENT_LIST_DIR}/interpolation/interpolation_set.h
	${CMAKE_CURRENT_LIST_DIR}/math/path_utils.h
	${CMAKE_CURRENT_LIST_DIR}/math/polarization_matrix.h
	${CMAKE_CURRENT_LIST_DIR}/noise/falpha_branch.h
	${CMAKE_CURRENT_LIST_DIR}/noise/noise_generators.h
	${CMAKE_CURRENT_LIST_DIR}/serial/hdf5_handler.h
	${CMAKE_CURRENT_LIST_DIR}/serial/kml_generator.h
	${CMAKE_CURRENT_LIST_DIR}/serial/libxml_wrapper.h
	${CMAKE_CURRENT_LIST_DIR}/serial/pulse_factory.h
	${CMAKE_CURRENT_LIST_DIR}/serial/receiver_export.h
	${CMAKE_CURRENT_LIST_DIR}/serial/xml_parser.h
	${CMAKE_CURRENT_LIST_DIR}/signal/dsp_filters.h
	${CMAKE_CURRENT_LIST_DIR}/signal/radar_signal.h
	${CMAKE_CURRENT_LIST_DIR}/timing/prototype_timing.h
	${CMAKE_CURRENT_LIST_DIR}/timing/timing.h
)

# --- Source Files ---
set(LIBFERS_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/api.cpp
	${CMAKE_CURRENT_LIST_DIR}/core/logging.cpp
	${CMAKE_CURRENT_LIST_DIR}/core/sim_threading.cpp
	${CMAKE_CURRENT_LIST_DIR}/core/thread_pool.cpp
	${CMAKE_CURRENT_LIST_DIR}/core/world.cpp
	${CMAKE_CURRENT_LIST_DIR}/antenna/antenna_factory.cpp
	${CMAKE_CURRENT_LIST_DIR}/interpolation/interpolation_filter.cpp
	${CMAKE_CURRENT_LIST_DIR}/interpolation/interpolation_set.cpp
	${CMAKE_CURRENT_LIST_DIR}/math/geometry_ops.cpp
	${CMAKE_CURRENT_LIST_DIR}/math/path.cpp
	${CMAKE_CURRENT_LIST_DIR}/math/polarization_matrix.cpp
	${CMAKE_CURRENT_LIST_DIR}/math/rotation_path.cpp
	${CMAKE_CURRENT_LIST_DIR}/noise/falpha_branch.cpp
	${CMAKE_CURRENT_LIST_DIR}/noise/noise_generators.cpp
	${CMAKE_CURRENT_LIST_DIR}/radar/radar_obj.cpp
	${CMAKE_CURRENT_LIST_DIR}/radar/receiver.cpp
	${CMAKE_CURRENT_LIST_DIR}/radar/target.cpp
	${CMAKE_CURRENT_LIST_DIR}/radar/transmitter.cpp
	${CMAKE_CURRENT_LIST_DIR}/serial/hdf5_handler.cpp
	${CMAKE_CURRENT_LIST_DIR}/serial/kml_generator.cpp
	${CMAKE_CURRENT_LIST_DIR}/serial/libxml_wrapper.cpp
	${CMAKE_CURRENT_LIST_DIR}/serial/pulse_factory.cpp
	${CMAKE_CURRENT_LIST_DIR}/serial/receiver_export.cpp
	${CMAKE_CURRENT_LIST_DIR}/serial/response.cpp
	${CMAKE_CURRENT_LIST_DIR}/serial/xml_parser.cpp
	${CMAKE_CURRENT_LIST_DIR}/signal/dsp_filters.cpp
	${CMAKE_CURRENT_LIST_DIR}/signal/radar_signal.cpp
	${CMAKE_CURRENT_LIST_DIR}/timing/prototype_timing.cpp
	${CMAKE_CURRENT_LIST_DIR}/timing/timing.cpp
)
