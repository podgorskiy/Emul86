cmake_minimum_required(VERSION 3.2)

project(Emul86)
set(EXECUTABLE_NAME "Emul86")

##############################################################
# Setup options for submodules
##############################################################
option (USE_MSVC_RUNTIME_LIBRARY_DLL OFF)
set(SKIP_INSTALL_ALL ON CACHE BOOL "Skip install" FORCE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)

##############################################################
# Setup compiler options and configurations
##############################################################

if(MSVC)
	set(COMMON_OPTIONS /W3 /wd4996 /we4239 /we4002 /we4700 -DGUID_WINDOWS /EHsc /MP)
	set(DEBUG_OPTIONS /MTd /Od /Zi)
	set(RELEASE_OPTIONS /MT /Ox /fp:fast /GL /GR- -DUSE_ARCHIVE)
elseif(APPLE)
	set(COMMON_OPTIONS)
	set(DEBUG_OPTIONS -g)
	set(RELEASE_OPTIONS -Os)
else()
	set(COMMON_OPTIONS)
	set(DEBUG_OPTIONS -g)
	set(RELEASE_OPTIONS -Os)
endif()

set(DEBUG_OPTIONS ${DEBUG_OPTIONS} ${COMMON_OPTIONS})
set(RELEASE_OPTIONS ${RELEASE_OPTIONS} ${COMMON_OPTIONS})

set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /LTCG")

add_compile_options(
  "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>"
  "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /INCREMENTAL:NO")
endif()

set(CMAKE_CONFIGURATION_TYPES  Release Debug)

add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
add_definitions(-DWINVER=0x0601)
add_definitions(-D_WIN32_WINNT=0x0601)
add_definitions(-DNTDDI_VERSION=0x06010000)

if (CMAKE_VERSION VERSION_LESS "3.1")
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    	set (CMAKE_CXX_FLAGS "--std=c++14 ${CMAKE_CXX_FLAGS}")
	endif ()
else ()
	set (CMAKE_CXX_STANDARD 14)
endif ()

##############################################################
# Adding libraries and subdirectories
##############################################################

set(GLM_H_PATH ${CMAKE_SOURCE_DIR}/libs/glm )

set(LZ4_DIR libs/lz4/lib/)
set(LZ4_LIBRARY lz4)
set(SOURCES_LZ4 ${LZ4_DIR}lz4.c ${LZ4_DIR}lz4hc.c ${LZ4_DIR}lz4.h ${LZ4_DIR}lz4hc.h ${LZ4_DIR}xxhash.c ${LZ4_DIR}xxhash.h)

add_library(lz4 ${SOURCES_LZ4})

##############################################################
# Includes
##############################################################
include_directories(libs/glm)
include_directories(libs/lz4/lib)
include_directories(sources)

include_directories("${PROJECT_BINARY_DIR}")

##############################################################
# Main sources
##############################################################
file(GLOB_RECURSE SOURCES sources/*.cpp sources/*.h sources/*.cc sources/*.proto)


##############################################################
# Targets
##############################################################
add_executable(Emul86 ${SOURCES})

##############################################################
# Linkage
##############################################################
target_link_libraries(Emul86 lz4)
