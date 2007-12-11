# - Find python libraries

# The copyright notice applies to parts of this source, but it has been modified.
# Adding this notice and clearly noting that this file is a modified version of the original is required to comply with the given licence.

#Copyright (c) 1993-2007 Ken Martin, Will Schroeder, Bill Lorensen 
#All rights reserved.


# This module finds if Python is installed and determines where the
# include files and libraries are. It also determines what the name of
# the library is. This code sets the following variables:
#
#  PYTHONLIBS_FOUND     = have the Python libs been found
#  PYTHON_LIBRARIES     = path to the python library
#  PYTHON_INCLUDE_PATH  = path to where Python.h is found
#  PYTHON_DEBUG_LIBRARIES = path to the debug library
#

INCLUDE(CMakeFindFrameworks)

# FIND_PACKAGE_HANDLE_STANDARD_ARGS(NAME (DEFAULT_MSG|"Custom failure message") VAR1 ... )
#    This macro is intended to be used in FindXXX.cmake modules files.
#    It handles the REQUIRED and QUIET argument to FIND_PACKAGE() and
#    it also sets the <UPPERCASED_NAME>_FOUND variable.
#    The package is found if all variables listed are TRUE.
#    Example:
#
#    FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibXml2 DEFAULT_MSG LIBXML2_LIBRARIES LIBXML2_INCLUDE_DIR)
#
#    LibXml2 is considered to be found, if both LIBXML2_LIBRARIES and 
#    LIBXML2_INCLUDE_DIR are valid. Then also LIBXML2_FOUND is set tto TRUE.
#    If it is not found and REQUIRED was used, it fails with FATAL_ERROR, 
#    independent whether QUIET was used or not.
#    If it is found, the location is reported using the VAR1 argument, so 
#    here a message "Found LibXml2: /usr/lib/libxml2.so" will be printed out.
#    If the second argument is DEFAULT_MSG, the message in the failure case will 
#    be "Could NOT find LibXml2", if you don't like this message you can specify
#    your own custom failure message there.

MACRO(FIND_PACKAGE_HANDLE_STANDARD_ARGS _NAME _FAIL_MSG _VAR1 )

  IF("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")
    SET(_FAIL_MESSAGE "Could NOT find ${_NAME}")
  ELSE("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")
    SET(_FAIL_MESSAGE "${_FAIL_MSG}")
  ENDIF("${_FAIL_MSG}" STREQUAL "DEFAULT_MSG")

  STRING(TOUPPER ${_NAME} _NAME_UPPER)

  SET(${_NAME_UPPER}_FOUND TRUE)
  IF(NOT ${_VAR1})
    SET(${_NAME_UPPER}_FOUND FALSE)
  ENDIF(NOT ${_VAR1})

  FOREACH(_CURRENT_VAR ${ARGN})
    IF(NOT ${_CURRENT_VAR})
      SET(${_NAME_UPPER}_FOUND FALSE)
    ENDIF(NOT ${_CURRENT_VAR})
  ENDFOREACH(_CURRENT_VAR)
  
  MESSAGE(STATUS "found ${_NAME_UPPER}_FOUND ${${_NAME_UPPER}_FOUND}")

  IF (${_NAME_UPPER}_FOUND)
    IF (NOT ${_NAME}_FIND_QUIETLY)
        MESSAGE(STATUS "Found ${_NAME}: ${${_VAR1}}")
    ENDIF (NOT ${_NAME}_FIND_QUIETLY)
  ELSE (${_NAME_UPPER}_FOUND)
    IF (${_NAME}_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "${_FAIL_MESSAGE}")
    ELSE (${_NAME}_FIND_REQUIRED)
      IF (NOT ${_NAME}_FIND_QUIETLY)
        MESSAGE(STATUS "${_FAIL_MESSAGE}")
      ENDIF (NOT ${_NAME}_FIND_QUIETLY)
    ENDIF (${_NAME}_FIND_REQUIRED)
  ENDIF (${_NAME_UPPER}_FOUND)
ENDMACRO(FIND_PACKAGE_HANDLE_STANDARD_ARGS)

# Search for the python framework on Apple.
CMAKE_FIND_FRAMEWORKS(Python)

FOREACH(_CURRENT_VERSION 2.5 2.4 2.3 2.2 2.1 2.0 1.6 1.5)
  STRING(REPLACE "." "" _CURRENT_VERSION_NO_DOTS ${_CURRENT_VERSION})
  IF(WIN32)
    FIND_LIBRARY(PYTHON_DEBUG_LIBRARY
      NAMES python${_CURRENT_VERSION_NO_DOTS}_d python
      PATHS
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs/Debug
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs )
  ENDIF(WIN32)

  FIND_LIBRARY(PYTHON_LIBRARY
    NAMES python${_CURRENT_VERSION_NO_DOTS} python${_CURRENT_VERSION}
    PATHS
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
    PATH_SUFFIXES
      python${_CURRENT_VERSION}/config
    # Avoid finding the .dll in the PATH.  We want the .lib.
    NO_SYSTEM_ENVIRONMENT_PATH
  )

  SET(PYTHON_FRAMEWORK_INCLUDES)
  IF(Python_FRAMEWORKS AND NOT PYTHON_INCLUDE_PATH)
    FOREACH(dir ${Python_FRAMEWORKS})
      SET(PYTHON_FRAMEWORK_INCLUDES ${PYTHON_FRAMEWORK_INCLUDES}
        ${dir}/Versions/${_CURRENT_VERSION}/include/python${_CURRENT_VERSION})
    ENDFOREACH(dir)
  ENDIF(Python_FRAMEWORKS AND NOT PYTHON_INCLUDE_PATH)

  FIND_PATH(PYTHON_INCLUDE_PATH
    NAMES Python.h
    PATHS
      ${PYTHON_FRAMEWORK_INCLUDES}
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/include
    PATH_SUFFIXES
      python${_CURRENT_VERSION}
  )
  
ENDFOREACH(_CURRENT_VERSION)

MARK_AS_ADVANCED(
  PYTHON_DEBUG_LIBRARY
  PYTHON_LIBRARY
  PYTHON_INCLUDE_PATH
)

# Python Should be built and installed as a Framework on OSX
IF(Python_FRAMEWORKS)
  # If a framework has been selected for the include path,
  # make sure "-framework" is used to link it.
  IF("${PYTHON_INCLUDE_PATH}" MATCHES "Python\\.framework")
    SET(PYTHON_LIBRARY "")
    SET(PYTHON_DEBUG_LIBRARY "")
  ENDIF("${PYTHON_INCLUDE_PATH}" MATCHES "Python\\.framework")
  IF(NOT PYTHON_LIBRARY)
    SET (PYTHON_LIBRARY "-framework Python" CACHE FILEPATH "Python Framework" FORCE)
  ENDIF(NOT PYTHON_LIBRARY)
  IF(NOT PYTHON_DEBUG_LIBRARY)
    SET (PYTHON_DEBUG_LIBRARY "-framework Python" CACHE FILEPATH "Python Framework" FORCE)
  ENDIF(NOT PYTHON_DEBUG_LIBRARY)
ENDIF(Python_FRAMEWORKS)

# We use PYTHON_LIBRARY and PYTHON_DEBUG_LIBRARY for the cache entries
# because they are meant to specify the location of a single library.
# We now set the variables listed by the documentation for this
# module.
SET(PYTHON_LIBRARIES "${PYTHON_LIBRARY}")
SET(PYTHON_DEBUG_LIBRARIES "${PYTHON_DEBUG_LIBRARY}")


FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonLibs DEFAULT_MSG PYTHON_LIBRARIES PYTHON_INCLUDE_PATH)

# make sure that the list of python modules is generated new every time cmake runs
# and not appended to the list of the previous cmake run
GET_TARGET_PROPERTY(_FIND_PYTHONLIBS_ALREADY_INCLUDED __FindPythonLibsHelper TYPE)
IF(NOT _FIND_PYTHONLIBS_ALREADY_INCLUDED)
  ADD_CUSTOM_TARGET(__FindPythonLibsHelper)  # so if this file is included several times the next time GET_TARGET_PROPERTIES() will work
  FILE(REMOVE ${CMAKE_BINARY_DIR}/CMakeFiles/PyModulesList.txt)
  FILE(REMOVE ${CMAKE_BINARY_DIR}/CMakeFiles/PyStaticModulesList.txt)
ENDIF(NOT _FIND_PYTHONLIBS_ALREADY_INCLUDED)


MACRO(PYTHON_ADD_MODULE _NAME )
  OPTION(PYTHON_ENABLE_MODULE_${_NAME} "Add module ${_NAME}" TRUE)

  IF(PYTHON_ENABLE_MODULE_${_NAME})
    OPTION(PYTHON_MODULE_${_NAME}_SHARED "Add module ${_NAME} shared" FALSE)
    IF(PYTHON_MODULE_${_NAME}_SHARED)
      SET(PY_MODULE_TYPE MODULE)
    ELSE(PYTHON_MODULE_${_NAME}_SHARED)
      SET(PY_MODULE_TYPE STATIC)
      IF(EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/PyStaticModulesList.txt")
        FILE(APPEND "${CMAKE_BINARY_DIR}/CMakeFiles/PyStaticModulesList.txt" ";${_NAME}")
      ELSE(EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/PyStaticModulesList.txt")
        FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/PyStaticModulesList.txt" "${_NAME}")
      ENDIF(EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/PyStaticModulesList.txt")
    ENDIF(PYTHON_MODULE_${_NAME}_SHARED)

    IF(EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/PyModulesList.txt")
      FILE(APPEND "${CMAKE_BINARY_DIR}/CMakeFiles/PyModulesList.txt" ";${_NAME}")
    ELSE(EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/PyModulesList.txt")
      FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/PyModulesList.txt" "${_NAME}")
    ENDIF(EXISTS "${CMAKE_BINARY_DIR}/CMakeFiles/PyModulesList.txt")
    ADD_LIBRARY(${_NAME} ${PY_MODULE_TYPE} ${ARGN})
#    TARGET_LINK_LIBRARIES(${_NAME} ${PYTHON_LIBRARIES})
  ENDIF(PYTHON_ENABLE_MODULE_${_NAME})
ENDMACRO(PYTHON_ADD_MODULE)


MACRO(PYTHON_WRITE_MODULES_HEADER _filename)
  FILE(READ "${CMAKE_BINARY_DIR}/CMakeFiles/PyStaticModulesList.txt" PY_STATIC_MODULES_LIST)
  FILE(READ "${CMAKE_BINARY_DIR}/CMakeFiles/PyModulesList.txt"       PY_MODULES_LIST)
  message(STATUS "modules: ${PY_MODULES_LIST}")
  message(STATUS "static modules: ${PY_STATIC_MODULES_LIST}")
  GET_FILENAME_COMPONENT(_name "${_filename}" NAME)
  STRING(REPLACE "." "_" _name "${_name}")
  STRING(TOUPPER ${_name} _name)
  FILE(WRITE ${_filename} "/*Created by cmake, do not edit, changes will be lost*/\n")
  FILE(APPEND ${_filename} 
"#ifndef ${_name}
#define ${_name}

#include <Python.h>

#ifdef __cplusplus
extern \"C\" {
#endif /* __cplusplus */

")

  FOREACH(_currentModule ${PY_MODULES_LIST})
    FILE(APPEND ${_filename} "extern void init${PYTHON_MODULE_PREFIX}${_currentModule}(void);\n\n")
  ENDFOREACH(_currentModule ${PY_MODULES_LIST})

  FILE(APPEND ${_filename} 
"#ifdef __cplusplus
}
#endif /* __cplusplus */

")


  FOREACH(_currentModule ${PY_MODULES_LIST})
    FILE(APPEND ${_filename} "int CMakeLoadPythonModule_${_currentModule}(void) \n{\n  return PyImport_AppendInittab(\"${PYTHON_MODULE_PREFIX}${_currentModule}\", init${PYTHON_MODULE_PREFIX}${_currentModule});\n}\n\n")
  ENDFOREACH(_currentModule ${PY_MODULES_LIST})

  FILE(APPEND ${_filename} "#ifndef EXCLUDE_LOAD_ALL_FUNCTION\nvoid CMakeLoadAllPythonModules(void)\n{\n")
  FOREACH(_currentModule ${PY_STATIC_MODULES_LIST})
    FILE(APPEND ${_filename} "  CMakeLoadPythonModule_${_currentModule}();\n")
  ENDFOREACH(_currentModule ${PY_STATIC_MODULES_LIST})
  FILE(APPEND ${_filename} "}\n#endif\n\n#endif\n")
ENDMACRO(PYTHON_WRITE_MODULES_HEADER)
