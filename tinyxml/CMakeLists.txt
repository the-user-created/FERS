INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
ADD_DEFINITIONS(-DTIXML_USE_STL)

########### next target ###############

SET(tinyxml_STAT_SRCS
   tinyxml.cpp
   tinyxmlerror.cpp
   tinyxmlparser.cpp
   )

ADD_LIBRARY(tinyxml STATIC ${tinyxml_STAT_SRCS})

########### install files ###############
