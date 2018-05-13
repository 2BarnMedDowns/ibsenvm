cmake_minimum_required (VERSION 3.1)
project (ibsen VERSION 0.0.1 LANGUAGES C)

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -g -DDEBUG")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

# Set source files
set (noravm_source "${PROJECT_SOURCE_DIR}/src/vm.c")
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

# Create Nora VM module
add_library(noravm MODULE ${noravm_source})
target_compile_definitions (noravm PRIVATE NORAVM_ID_STRING="noravm-${PROJECT_VERSION}" NORAVM_ENTRY_ADDR=0)


# Create test executable
add_executable (test "${source}")
target_compile_definitions (test PRIVATE IBSEN_VERSION="${PROJECT_VERSION}" NORAVM_ID_STRING="ibsen-${PROJECT_VERSION}" NORAVM_ENTRY_ADDR=0x1000000ULL)

