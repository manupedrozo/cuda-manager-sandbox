cmake_minimum_required (VERSION 2.8.11)

if (NOT MANGO_ROOT) 
    message(FATAL_ERROR "You have to specify -DMANGO_ROOT=\"/path/to/mango\"!")
endif (NOT MANGO_ROOT)

add_subdirectory(cuda_compiler)
add_subdirectory(cuda_manager)
