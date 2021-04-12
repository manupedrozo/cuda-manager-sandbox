cmake_minimum_required (VERSION 2.8.11)

project(compiler LANGUAGES CXX CUDA)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose debug or release" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

message(STATUS "[Compiler] Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")
message(STATUS "[Compiler] Build type: ${CMAKE_BUILD_TYPE}")

set(MANGO_ROOT /opt/mango)
set(INCLUDE_DIR ${MANGO_ROOT}/include/cuda_compiler)
set(LIB_DIR ${MANGO_ROOT}/lib/cuda_compiler)
set(EXPORT_DIR ${MANGO_ROOT}/lib/cmake/cuda_compiler)
set(TOOL_INSTALL_DIR ${MANGO_ROOT}/usr/bin/cuda_compiler)

set(SOURCES cuda_compiler.cpp)
set(HEADERS cuda_compiler.h)

find_library(CUDA_LIBRARY cuda ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
find_library(NVRTC_LIBRARY nvrtc ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})

message("CUDA_LIBRARY: ${CUDA_LIBRARY}")
message("NVRTC_LIBRARY: ${NVRTC_LIBRARY}")
message("Include directories: ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")

install(FILES ${HEADERS} DESTINATION ${INCLUDE_DIR})

# Standalone compiler library
add_library(cuda_compiler SHARED ${SOURCES} ${HEADERS})
target_link_libraries(cuda_compiler PRIVATE ${CUDA_LIBRARY} ${NVRTC_LIBRARY})
target_include_directories(cuda_compiler PRIVATE ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

target_include_directories(cuda_compiler PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INCLUDE_DIR}>)

install(
    TARGETS cuda_compiler 
    EXPORT cuda_compilerConfig 
    LIBRARY 
    DESTINATION ${LIB_DIR})

install(EXPORT cuda_compilerConfig DESTINATION ${EXPORT_DIR})

# Standalone compiler installation to compile single kernels
add_executable(cuda_compiler_tool ${SOURCES} compiler_main.cpp)
target_link_libraries(cuda_compiler_tool PRIVATE ${CUDA_LIBRARY} ${NVRTC_LIBRARY})
target_include_directories(cuda_compiler_tool PRIVATE ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
install(TARGETS cuda_compiler_tool DESTINATION ${TOOL_INSTALL_DIR})


