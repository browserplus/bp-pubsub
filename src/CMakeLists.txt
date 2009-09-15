#####
# A CMake build file for the HelloWorld BrowserPlus Service.
#
# Copyright 2009, Yahoo!
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in
# the documentation and/or other materials provided with the
# distribution.
#
# 3. Neither the name of Yahoo! nor the names of its
# contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Original Author: Lloyd Hilaiel
#####
 
cmake_minimum_required(VERSION 2.6)

SET(frameworkDir "${CMAKE_CURRENT_SOURCE_DIR}/../external/bp-service-framework")
INCLUDE(${frameworkDir}/build_support/BuildConfigs.cmake)
 
SET(serviceName PubSub)
 
PROJECT(${serviceName}Service)
 
SET(outputDir "${CMAKE_CURRENT_BINARY_DIR}/${serviceName}")
 
# default to a debug build. This gets us symbols by default under
# the cmake makefile generator
IF (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Debug")
ENDIF ()
 
INCLUDE_DIRECTORIES("${frameworkDir}/include")



#SET(SRCS pubsub.cpp)
#SET(HDRS pubsub.h listener.h)
FILE(GLOB SRCS "*.cpp")
FILE(GLOB HDRS "*.h")
SET(OS_SRCS)
SET(EXT_SRCS)


#SET(SRCS service.cpp util/fileutil.cpp)
#SET(HDRS util/bpsync.hh util/bpthread.hh)
#IF (WIN32)
#   FILE(GLOB OS_SRCS "util/*_Windows.cpp")
#ELSE()
#   FILE(GLOB OS_SRCS "util/*_Darwin.cpp" "util/*_UNIX.cpp")
#ENDIF ()
 
ADD_LIBRARY(${serviceName} MODULE ${SRCS} ${HDRS} ${OS_SRCS} ${EXT_SRCS})

 
# add required OS libs here
SET(OSLIBS)

#TARGET_LINK_LIBRARIES(${serviceName} easylzma_s ${OSLIBS})

# create service directory
FILE(MAKE_DIRECTORY ${outputDir})
 
# copy in manifest
GET_TARGET_PROPERTY(loc ${serviceName} LOCATION)
GET_FILENAME_COMPONENT(ServiceLibrary "${loc}" NAME)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/manifest.json"
               "${outputDir}/manifest.json")
ADD_CUSTOM_COMMAND(TARGET ${serviceName} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           \"${loc}\" \"${outputDir}\")
IF (APPLE)
  ADD_CUSTOM_COMMAND(TARGET ${serviceName} POST_BUILD
                     COMMAND strip -x \"${outputDir}/${ServiceLibrary}\")
ENDIF (APPLE)
