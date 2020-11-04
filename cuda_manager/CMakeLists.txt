cmake_minimum_required (VERSION 2.8.11)

project(cuda_sandbox LANGUAGES CXX CUDA)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(MANGO_ROOT /opt/mango)
set(LIB_DIR ${MANGO_ROOT}/lib/cuda_manager)
set(INCLUDE_DIR ${MANGO_ROOT}/include/cuda_manager)
set(EXPORT_DIR ${MANGO_ROOT}/lib/cmake/cuda_manager)

set(SOURCES cuda_manager.cpp cuda_compiler.cpp cuda_argument_parser.cpp cuda_memory_manager.cpp cuda_api.cpp)
set(HEADERS cuda_common.h cuda_argument_parser.h cuda_compiler.h cuda_manager.h cuda_memory_manager.h cuda_api.h kernel_arguments.h)

add_library(cuda_manager SHARED ${SOURCES} ${HEADERS})
add_executable(launch_kernel_test main.cpp)

find_library(CUDA_LIBRARY cuda ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
find_library(NVRTC_LIBRARY nvrtc ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})

message("CUDA_LIBRARY: ${CUDA_LIBRARY}")
message("NVRTC_LIBRARY: ${NVRTC_LIBRARY}")
message("Include directories: ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}")

target_link_libraries(launch_kernel_test PRIVATE ${CUDA_LIBRARY} ${NVRTC_LIBRARY} cuda_manager)
target_link_libraries(cuda_manager PRIVATE ${CUDA_LIBRARY} ${NVRTC_LIBRARY})

target_include_directories(launch_kernel_test PRIVATE ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

target_include_directories(cuda_manager PUBLIC ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
target_include_directories(cuda_manager PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INCLUDE_DIR}>)

install(TARGETS cuda_manager EXPORT cuda_managerConfig LIBRARY DESTINATION ${LIB_DIR})

install(FILES ${HEADERS} DESTINATION ${INCLUDE_DIR})

install(EXPORT cuda_managerConfig DESTINATION ${EXPORT_DIR})

configure_file(saxpy.cu saxpy.cu COPYONLY)
