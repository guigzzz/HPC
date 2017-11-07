## This was contributed by https://github.com/farrell236 in 2014.

## ==============================================================================
##
##  CMake project settings
##
## ==============================================================================

## Name of the project handled by CMake
project (HPCE-2016-CW3)

## Minimum required version of CMake to configure the project
cmake_minimum_required (VERSION 2.8)

## Enforced CMake policy
cmake_policy (VERSION 2.8)


## ==============================================================================
##
##  Configure Khronos OpenCL Libs
##
## ==============================================================================

set(OPENCL_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opencl_sdk/)

set(OPENCL_SDK_HEADERS ${OPENCL_SDK_DIR}/include/)

if(WIN32)
  set(OPENCL_SDK_LIB     ${OPENCL_SDK_DIR}/lib/windows/x86/OpenCL.lib)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OPENCL_SDK_LIB      ${OPENCL_SDK_DIR}/lib/windows/x86_64/OpenCL.lib)
  endif()
elseif(CYGWIN)
  set(OPENCL_SDK_LIB      ${OPENCL_SDK_DIR}/lib/cygwin/x86/libOpenCL.a)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(OPENCL_SDK_LIB      ${OPENCL_SDK_DIR}/lib/cygwin/x86_64/libOpenCL.a)
  endif()
else()
    # TODO : If it fails, you may need to add a new directory to the hint
    #list for your system, or explicitly state where it is.
    find_library(OPENCL_SDK_LIB NAMES OpenCL libOpenCL
       HINTS /usr/lib/x86_64-linux-gnu /usr/local/lib /usr/lib/x86-linux-gnu
    )
endif()


## ==============================================================================
##
##  Configure Default Coursework Files
##
## ==============================================================================

set(COURSEWORK_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/)

set(HEAT_HPP ${CMAKE_CURRENT_SOURCE_DIR}/include/heat.hpp)
set(HEAT_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/heat.cpp)

include_directories(${COURSEWORK_HEADER} ${OPENCL_SDK_HEADERS})

add_executable(test_opencl   ${HEAT_HPP}  ${HEAT_CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_opencl.cpp)
add_executable(make_world    ${HEAT_HPP}  ${HEAT_CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/src/make_world.cpp)
add_executable(render_world  ${HEAT_HPP}  ${HEAT_CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/src/render_world.cpp)
add_executable(step_world    ${HEAT_HPP}  ${HEAT_CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/src/step_world.cpp)

target_link_libraries(test_opencl ${OPENCL_SDK_LIB})
target_link_libraries(make_world ${OPENCL_SDK_LIB})
target_link_libraries(render_world ${OPENCL_SDK_LIB})
target_link_libraries(step_world ${OPENCL_SDK_LIB})

## ==============================================================================
##
##  Configure Additional Coursework Files
##
## ==============================================================================

## TODO : Change this param to your IC username
set(LOGIN_ID "gr1714")

set(USR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/${LOGIN_ID})
file(GLOB USR_SRC RELATIVE ${USR_SRC_DIR} ${USR_SRC_DIR}/*.cpp)

foreach(SRC_FILE ${USR_SRC})
  string(REPLACE ".cpp" "" EXECUTABLE ${SRC_FILE})
  add_executable(${EXECUTABLE} ${HEAT_HPP} ${HEAT_CPP} ${USR_SRC_DIR}/${SRC_FILE})
  target_link_libraries(${EXECUTABLE} ${OPENCL_SDK_LIB})
endforeach(SRC_FILE ${USR_SRC})


## ==============================================================================
##
##  Compiler Flags
##
## ==============================================================================

# As suggested by @hamish-milne
#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -std=c++11 -o2")
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -std=gnu++11 -o2")
## Get rid of DLL errors
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

## ==============================================================================
##
##  Configuration summary
##
## ==============================================================================

message (STATUS "------------------------------------------------------------"  )
message (STATUS "[HPCE-2017-CW3] Configuration summary."                        )
message (STATUS "------------------------------------------------------------"  )
message (STATUS " System configuration:"                                        )
message (STATUS " .. Processor type ............. = ${CMAKE_SYSTEM_PROCESSOR}"  )
message (STATUS " .. CMake executable ........... = ${CMAKE_COMMAND}"           )
message (STATUS " .. CMake version .............. = ${CMAKE_VERSION}"           )
message (STATUS " .. System name ................ = ${CMAKE_SYSTEM}"            )
message (STATUS " .. C++ compiler ............... = ${CMAKE_CXX_COMPILER}"      )
message (STATUS " .. C compiler ................. = ${CMAKE_C_COMPILER}"        )
message (STATUS " .. size(void*) ................ = ${CMAKE_SIZEOF_VOID_P}"     )

IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/${LOGIN_ID})
  message (STATUS " Additional Coursework Files:")
  foreach( SRC_FILE ${USR_SRC} )
    message (STATUS " .. ${SRC_FILE}")
  endforeach( SRC_FILE ${USR_SRC} )
endif()

message (STATUS "------------------------------------------------------------"  )

