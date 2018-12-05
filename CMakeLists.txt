# Required by CMake 2.7+
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
      cmake_policy(SET CMP0002 NEW)
      IF(APPLE)
        cmake_policy(SET CMP0042 NEW) #MacOS rpath
      ENDIF()
endif(COMMAND cmake_policy)
if( NOT CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE Release CACHE STRING
       "Choose the type of build, options are: None Debug Release RelWithDebInfo
MinSizeRel."
       FORCE )
endif()

# add flags
if (NOT CONFIGURED_ONCE)
    IF (WIN32)
        SET(CONDITIONAL_CFLAGS "/bigobj /EHsc ${CONDITIONAL_CFLAGS}")
    ENDIF()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall ${CONDITIONAL_CFLAGS}"
        CACHE STRING "Flags used by the compiler during all build types." FORCE)
    SET(CONFIGURED_ONCE TRUE CACHE STRING "Configured at least once?")
endif()
MESSAGE("Current CXX: " ${CMAKE_CXX_COMPILER})
if (APPLE)
    add_definitions(-DMACOS)
endif()

# Start project
PROJECT(pb_cpp2py)

# Creates a compiler_commands.json file for all compilations.
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#OPTION(BUILD_SHARED_LIBS "Build Shared Libraries" ON)
# All binary files go to "build" directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# att global compiler flags
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall" )

#find_package(OpenCV REQUIRED)
#find_package(Boost COMPONENTS system filesystem program_options REQUIRED)

SET(PROJECT_ROOT ${CMAKE_SOURCE_DIR} )

find_package(Protobuf REQUIRED)

MESSAGE(${PROJECT_ROOT})
MESSAGE("HERE")
MESSAGE(${CMAKE_CURRENT_BINARY_DIR})
SET(PROTO_DEFINE_NAME soccer )
SET(PROTO_DEFINE_FULL_NAME ${PROTO_DEFINE_NAME}.proto )

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROJECT_ROOT}/protobuf-define/${PROTO_DEFINE_FULL_NAME})
set_source_files_properties(${PROTO_SRC} ${PROTO_HEADER} PROPERTIES GENERATED TRUE)

# Due to the directory structure of the main repo, source files have the lines
# #include <protobuf-define/${PROTO_DEFINE_NAME}.pb.h>
# PROTOBUF_GENERATE_CPP copies to CMAKE_CURRENT_BINARY_DIR without the option to
# do otherwise, so this rule copies the header to CMAKE_CURRENT_BUILD_DIR/protobuf-define,
# which, when combined with the build directory being an include directory, allows
# these paths to make sense.

# add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/protobuf-define/soccer.pb.h
# COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/protobuf-define/
# COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_CURRENT_BINARY_DIR}/protobuf-define/${PROTO_DEFINE_NAME}.pb.h ${CMAKE_CURRENT_BINARY_DIR}/protobuf-define/${PROTO_DEFINE_NAME}.pb.h
# MAIN_DEPENDENCY ${CMAKE_CURRENT_BINARY_DIR}/soccer.pb.h)
# add_custom_target(proto-headers echo Copying protobuf headers. DEPENDS
#   ${CMAKE_CURRENT_BINARY_DIR}/protobuf-define/${PROTO_DEFINE_NAME}.pb.h )

SET(INCLUDE_DIRS_TO_SET
    ${CWD_INCLUDE_DIRS}
    ${PROJECT_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_ROOT}
)

INCLUDE_DIRECTORIES(SYSTEM ${SYS_INCLUDE_DIRS_TO_SET})
INCLUDE_DIRECTORIES(${INCLUDE_DIRS_TO_SET})

ADD_SUBDIRECTORY(lib)
#ADD_SUBDIRECTORY(server)
