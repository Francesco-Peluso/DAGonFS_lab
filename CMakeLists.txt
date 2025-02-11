cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME DAGonFS)
project(${PROJECT_NAME})

option(USE_MPI "Use MPI for distributed memory." OFF)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMake)

find_package(FUSE3 REQUIRED)
include_directories(SYSTEM ${FUSE3_INCLUDE_DIR})

find_package(MPI)
if(MPI_C_FOUND)
  if (USE_MPI)
    message(STATUS "Using MPI for distributed memory.")
    add_definitions(-DUSE_MPI)
    include_directories(SYSTEM ${MPI_INCLUDE_PATH})
  endif()
endif()

if(WIN32)
  set(SHAREDIR ".")
  set(DOCDIR "doc")
elseif(APPLE)
  set(SHAREDIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")
  set(DOCDIR "share/doc/${PROJECT_NAME}")
  set(MANDIR "share/man")
elseif(UNIX) # Linux, BSD etc
  set(SHAREDIR "share/${PROJECT_NAME}")
  set(DOCDIR "share/doc/${PROJECT_NAME}")
  set(MANDIR "share/man")
endif()


add_subdirectory(doc)

#install(DIRECTORY sample_data DESTINATION "${SHAREDIR}")

#log4cplus
#find_package(LOG4CPLUS)
#include_directories(SYSTEM ${LOG4CPLUS_INCLUDE_DIR})
set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)
include_directories(SYSTEM ${EXTERNAL_INSTALL_LOCATION}/include)
include(ExternalProject)
link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
ExternalProject_Add(log4cplus
        #URL https://kumisystems.dl.sourceforge.net/project/log4cplus/log4cplus-stable/2.0.5/log4cplus-2.0.5.tar.gz
        URL https://github.com/log4cplus/log4cplus/releases/download/REL_2_1_1/log4cplus-2.1.1.tar.gz
        TIMEOUT 360
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ./configure --prefix=${EXTERNAL_INSTALL_LOCATION} CFLAGS=-fPIC CPPFLAGS=-I${EXTERNAL_INSTALL_LOCATION}/include/ LDFLAGS=-L${EXTERNAL_INSTALL_LOCATION}/lib/
        BUILD_COMMAND make
        INSTALL_COMMAND make install
)
set(LOG4CPLUS_LIBRARIES ${EXTERNAL_INSTALL_LOCATION}/lib/liblog4cplus.so)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(LOG4CPLUS_LIBRARIES ${EXTERNAL_INSTALL_LOCATION}/lib/liblog4cplus.dylib)
endif()

file(GLOB_RECURSE DAGONFS_CS_HEADER_FILES src/client-server/include/*.hpp)
file(GLOB_RECURSE DAGONFS_CS_SOURCE_FILES src/client-server/include/*.cpp)

file(GLOB_RECURSE DAGONFS_P2P_HEADER_FILES src/peer-to-peer/include/*.hpp)
file(GLOB_RECURSE DAGONFS_P2P_SOURCE_FILES src/peer-to-peer/include/*.cpp)

# Creation of two executables for two DAGonFS models
add_executable("${PROJECT_NAME}_CS.exe" src/client-server/main.cpp ${DAGONFS_CS_SOURCE_FILES} ${DAGONFS_CS_HEADER_FILES})
add_executable("${PROJECT_NAME}_P2P.exe" src/peer-to-peer/main.cpp ${DAGONFS_P2P_SOURCE_FILES} ${DAGONFS_P2P_HEADER_FILES})

# Creation of DAGonFS launcher
add_executable("${PROJECT_NAME}_Launcher" src/DAGonFS_Launcher.cpp)

# Compilation with required libraries (MPI, FUSE and log4cplus)
target_link_libraries("${PROJECT_NAME}_CS.exe" ${FUSE3_LIBRARIES} ${MPI_C_LIBRARIES} ${LOG4CPLUS_LIBRARIES} -lpthread)
target_link_libraries("${PROJECT_NAME}_P2P.exe" ${FUSE3_LIBRARIES} ${MPI_C_LIBRARIES} ${LOG4CPLUS_LIBRARIES} -lpthread)

# Specific definitions
target_compile_definitions(${PROJECT_NAME}_CS.exe PRIVATE FUSE_USE_VERSION=32 _FILE_OFFSET_BITS=64)
target_compile_definitions(${PROJECT_NAME}_P2P.exe PRIVATE FUSE_USE_VERSION=32 _FILE_OFFSET_BITS=64)

# Imposta C++ Standard
set_property(TARGET ${PROJECT_NAME}_CS.exe PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME}_P2P.exe PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME}_Launcher PROPERTY CXX_STANDARD 23)

# Installazione
install(TARGETS ${PROJECT_NAME}_CS.exe ${PROJECT_NAME}_P2P.exe ${PROJECT_NAME}_Launcher DESTINATION bin)

