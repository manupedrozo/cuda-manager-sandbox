cmake_minimum_required (VERSION 2.8.11)

set(MANGO_ROOT /opt/mango)

project(cuda_sandbox LANGUAGES CXX CUDA)

set(SOURCES cuda_manager.cpp cuda_compiler.cpp cuda_argument_parser.cpp cuda_memory_manager.cpp cuda_api.cpp)
set(HEADERS cuda_common.h cuda_argument_parser.h cuda_compiler.h cuda_manager.h cuda_memory_manager.h cuda_api.h)

add_library(cuda_manager SHARED ${SOURCES})
add_executable(launch_kernel_test main.cpp)

find_library(CUDA_LIBRARY cuda ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
find_library(NVRTC_LIBRARY nvrtc ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})

target_link_libraries(launch_kernel_test PRIVATE ${CUDA_LIBRARY} ${NVRTC_LIBRARY} cuda_manager)
target_link_libraries(cuda_manager PRIVATE ${CUDA_LIBRARY} ${NVRTC_LIBRARY})

target_include_directories(launch_kernel_test PRIVATE ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})
target_include_directories(cuda_manager PRIVATE ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

install (FILES ${HEADERS} DESTINATION ${MANGO_ROOT}/include/libcuda_manager/cuda_manager)

install (TARGETS cuda_manager LIBRARY DESTINATION ${MANGO_ROOT}/lib)

configure_file(saxpy.cu saxpy.cu COPYONLY)

