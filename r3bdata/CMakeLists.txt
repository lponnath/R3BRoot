# Create a library called "libR3BDATA" which includes the source files given in
# the array .
# The extension is already found.  Any number of sources could be listed here.

set(INCLUDE_DIRECTORIES
${ROOT_INCLUDE_DIR} 
${CMAKE_SOURCE_DIR}/geobase 
${CMAKE_SOURCE_DIR}/base 
${CMAKE_SOURCE_DIR}/r3bdata  
)

include_directories( ${INCLUDE_DIRECTORIES})

set(LINK_DIRECTORIES
${ROOT_LIBRARY_DIR}
${CMAKE_SOURCE_DIR}/base
)
 
link_directories( ${LINK_DIRECTORIES})

set(R3BDATA_SRCS
R3BMCStack.cxx
R3BMCTrack.cxx
calData/R3BCalPoint.cxx
caloData/R3BCaloPoint.cxx 
dchData/R3BDchPoint.cxx
gfiData/R3BGfiPoint.cxx 
landData/R3BLandPoint.cxx
mtofData/R3BmTofPoint.cxx 
tofData/R3BTofPoint.cxx
trackerData/R3BTraPoint.cxx)

if(RULE_CHECKER_FOUND)
CHECK_RULES("${R3BDATA_SRCS}" "${INCLUDE_DIRECTORIES}" R3BDATA_RULES)
endif(RULE_CHECKER_FOUND)

# fill list of header files from list of source files
# by exchanging the file extension
CHANGE_FILE_EXTENSION(*.cxx *.h R3BDATA_HEADERS "${R3BDATA_SRCS}")
SET( R3BDATA_HEADERS ${R3BDATA_HEADERS} R3BDetectorList.h )

set(R3BDATA_LINKDEF  R3BDataLinkDef.h)
set(R3BDATA_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/R3BDataDict.cxx)

ROOT_GENERATE_DICTIONARY("${R3BDATA_HEADERS}" "${R3BDATA_LINKDEF}" "${R3BDATA_DICTIONARY}" "${INCLUDE_DIRECTORIES}")

SET(R3BDATA_SRCS ${R3BDATA_SRCS} ${R3BDATA_DICTIONARY})

add_library(R3BData SHARED ${R3BDATA_SRCS})
target_link_libraries(R3BData ${ROOT_LIBRARIES})
set_target_properties(R3BData PROPERTIES ${FAIRROOT_LIBRARY_PROPERTIES})
################ install ###################
install(TARGETS R3BData DESTINATION ${CMAKE_BINARY_DIR}/lib)