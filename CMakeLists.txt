cmake_minimum_required(VERSION 3.2)

project(Emul86)
set(EXECUTABLE_NAME "Emul86")

##############################################################
# Setup options for submodules
##############################################################
option (GLFW_BUILD_EXAMPLES OFF)
option (GLFW_BUILD_TESTS OFF)
option (GLFW_INSTALL OFF)
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
# Generating OpenGL profile
##############################################################
message("Executing gl3w_gen.py")
execute_process(COMMAND python gl3w_gen.py WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/gl3w)

##############################################################
# Adding libraries and subdirectories
##############################################################
add_subdirectory (libs/glfw)
add_subdirectory (libs/gl3w)

##############################################################
# Includes
##############################################################
include_directories(libs/glm)
include_directories(libs/glfw/include)
include_directories(libs/gl3w/include)
include_directories(libs/SimpleText/include)
include_directories(sources)
include_directories("${PROJECT_BINARY_DIR}")
include_directories(libs/imgui)

find_package(OpenGL)
if (OPENGL_FOUND)
	message("OpenGL Correctly Found")
	include_directories(${OPENGL_INCLUDE_DIR})
else (OPENGL_FOUND)
	message("OpenGL environment missing")
endif (OPENGL_FOUND)

#####################################################################
# imgui
#####################################################################
set(IMGUI_DIR libs/imgui)
file(GLOB IMGUI_SOURCES ${IMGUI_DIR}/*.cpp ${IMGUI_DIR}/*.h )
add_library(imgui ${IMGUI_SOURCES})

##############################################################
# Main sources
##############################################################
file(GLOB_RECURSE SOURCES sources/*.cpp sources/*.h)

##############################################################
# Targets
##############################################################
add_executable(Emul86 ${SOURCES})

##############################################################
# Linkage
##############################################################
target_link_libraries(Emul86 glfw gl3w imgui ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
