cmake_minimum_required (VERSION 3.1)
project (ibsen VERSION 0.0.1 LANGUAGES C)

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -g -DDEBUG")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

set (vm_data_size "0x40000000")
set (vm_data_start "0x40000000")
set (vm_data_page "0x100")

# Set source files
set (vm_source "${PROJECT_SOURCE_DIR}/vm/vm.c")
file (GLOB source "${PROJECT_SOURCE_DIR}/src/*.c")


if (NOT APPLE)
	message (STATUS "loonix")
    list (REMOVE_ITEM source "${PROJECT_SOURCE_DIR}/src/macho.c")
else ()
	message(STATUS "applel")
	list (REMOVE_ITEM source "${PROJECT_SOURCE_DIR}/src/elf.c")
endif ()

# Set include directory
include_directories ("${PROJECT_BINARY_DIR}/include" "${PROJECT_SOURCE_DIR}/include")

# Create VM module
add_library(vm STATIC ${vm_source})
target_compile_definitions (vm PRIVATE NORAVM_ID_STRING="vm-${PROJECT_VERSION}" NORAVM_MEM_ADDR=${vm_data_start})
target_compile_options (vm BEFORE PUBLIC -nostdlib)


# Create test executable
add_executable (tester "${source}")
target_compile_definitions (tester PUBLIC IBSEN_VERSION="${PROJECT_VERSION}" NORAVM_ID_STRING="ibsen-${PROJECT_VERSION}" NORAVM_MEM_ADDR=${vm_data_start} NORAVM_MEM_TOTAL_SIZE=${vm_data_size} NORAVM_MEM_PAGE_SIZE=${vm_data_page})
target_link_libraries (tester vm)

