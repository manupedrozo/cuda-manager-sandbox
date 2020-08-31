cmake_minimum_required (VERSION 2.8.11)

project(cuda_sandbox LANGUAGES CXX CUDA)

add_executable(launch_kernel_test main.cpp cuda_manager.cpp cuda_compiler.cpp)

find_library(CUDA_LIBRARY cuda ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
find_library(NVRTC_LIBRARY nvrtc ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})

target_link_libraries(launch_kernel_test PRIVATE ${CUDA_LIBRARY} ${NVRTC_LIBRARY})

target_include_directories(launch_kernel_test PRIVATE ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES})

configure_file(saxpy.cu saxpy.cu COPYONLY)