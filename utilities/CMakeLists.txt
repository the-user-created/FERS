INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )


########### next target ###############

SET(csv2antenna_SRCS
   csv2antenna.c
   )

ADD_EXECUTABLE(csv2antenna ${csv2antenna_SRCS})


########### install files ###############




#original Makefile.am contents follow:

#
#bin_PROGRAMS = csv2antenna
#csv2antenna_SOURCES = csv2antenna.c
