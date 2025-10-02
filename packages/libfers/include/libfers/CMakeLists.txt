# This file is the central manifest for the libfers public implementation. It defines:
#  - LIBFERS_PUBLIC_HEADERS: Headers for consumers of the library.

# --- Public Headers ---
set(LIBFERS_PUBLIC_HEADERS
	${CMAKE_CURRENT_LIST_DIR}/api.h
	${CMAKE_CURRENT_LIST_DIR}/antenna_factory.h
	${CMAKE_CURRENT_LIST_DIR}/config.h
	${CMAKE_CURRENT_LIST_DIR}/coord.h
	${CMAKE_CURRENT_LIST_DIR}/geometry_ops.h
	${CMAKE_CURRENT_LIST_DIR}/logging.h
	${CMAKE_CURRENT_LIST_DIR}/object.h
	${CMAKE_CURRENT_LIST_DIR}/parameters.h
	${CMAKE_CURRENT_LIST_DIR}/path.h
	${CMAKE_CURRENT_LIST_DIR}/platform.h
	${CMAKE_CURRENT_LIST_DIR}/portable_utils.h
	${CMAKE_CURRENT_LIST_DIR}/radar_obj.h
	${CMAKE_CURRENT_LIST_DIR}/receiver.h
	${CMAKE_CURRENT_LIST_DIR}/response.h
	${CMAKE_CURRENT_LIST_DIR}/rotation_path.h
	${CMAKE_CURRENT_LIST_DIR}/target.h
	${CMAKE_CURRENT_LIST_DIR}/transmitter.h
	${CMAKE_CURRENT_LIST_DIR}/world.h
)
