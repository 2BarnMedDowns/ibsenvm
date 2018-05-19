cmake_minimum_required (VERSION 3.1)
project (ibsen VERSION 0.0.1 LANGUAGES C)

# Common defines
set (start_addr "0x80000000" CACHE STRING "Start address of the data segment")


# Compiler flags
set (CMAKE_C_STANDARD 99)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -g -DDEBUG")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")


# Source files
set (source_root "${PROJECT_SOURCE_DIR}/src")                   # Source code root directory
file (GLOB vm_source "${PROJECT_SOURCE_DIR}/vm/*.c")            # Source code files to the Ibsen virtual machine
file (GLOB source "${source_root}/*.c")                         # Source code files
file (GLOB linux_source "${source_root}/linux/*.c")             # Part of source code that is Linux specific
file (GLOB macos_source "${source_root}/macos/*.c")             # Part of source code that is specific to macOS
file (GLOB linker_source "${PROJECT_SOURCE_DIR}/linker/*.c")    # Source code for linker


# Set include directory
include_directories ("${PROJECT_BINARY_DIR}/include" "${PROJECT_SOURCE_DIR}/include")


# Create standalone static library for Ibsen virtual machine
add_library (ibsenvm STATIC ${vm_source})
target_compile_definitions (ibsenvm PRIVATE IVM_ID_STRING="ibsenvm-${PROJECT_VERSION}" IVM_ENTRY=${start_addr})
target_compile_options (ibsenvm BEFORE PUBLIC -nostdlib)


# Create library
add_library (libivm SHARED ${source})
target_compile_definitions (libivm PUBLIC IVM_VERSION="${PROJECT_VERSION}" IVM_ID_STRING="ivm-${PROJECT_VERSION}" IVM_ENTRY=${start_addr})


# Add OS specific sources to target
if (NOT APPLE)
    message (STATUS "Compiling for Linux")
    target_sources (libivm PRIVATE "${linux_source}")
else ()
    message (STATUS "Compiling for macOS")
    target_sources (libivm PRIVATE "${macos_source}")
endif ()


# Create linker target
add_executable (linker ${linker_source})
target_link_libraries (linker libivm ibsenvm)
