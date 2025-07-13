cmake_minimum_required(VERSION 3.16)
project(kernel C ASM)

# Compiler flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Check if using Clang
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-target x86_64-unknown-none)
endif()

# Common compiler flags
add_compile_options(
    -Wall
    -Wextra
    -nostdinc
    -ffreestanding
    -fno-stack-protector
    -fno-stack-check
    -fno-PIC
    -ffunction-sections
    -fdata-sections
    -m64
    -march=x86-64
    -mno-80387
    -mno-mmx
    -mno-sse
    -mno-sse2
    -mno-red-zone
    -mcmodel=kernel
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/freestnd-c-hdrs-0bsd/include
    ${CMAKE_CURRENT_SOURCE_DIR}/cc-runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/limine-protocol/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_definitions(-DLIMINE_API_REVISION=2)

# Find source files
file(GLOB_RECURSE KERNEL_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB_RECURSE KERNEL_ASM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm")
file(GLOB_RECURSE KERNEL_S_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.S")
file(GLOB_RECURSE KERNEL_O_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.o")

# NASM rules
foreach(ASM_SOURCE ${KERNEL_ASM_SOURCES})
    get_filename_component(ASM_NAME ${ASM_SOURCE} NAME_WE)
    set(ASM_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${ASM_NAME}.o")
    add_custom_command(
        OUTPUT ${ASM_OUTPUT}
        COMMAND nasm -f elf64 -g -F dwarf -Wall ${ASM_SOURCE} -o ${ASM_OUTPUT}
        DEPENDS ${ASM_SOURCE}
    )
    list(APPEND KERNEL_OBJECTS ${ASM_OUTPUT})
endforeach()

# More preventing of recloning
add_custom_target(kernel-deps
    COMMAND test -d freestnd-c-hdrs-0bsd -a -d cc-runtime -a -d limine-protocol || ./get-deps
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Create kernel executable
add_executable(kernel ${KERNEL_C_SOURCES} ${KERNEL_S_SOURCES} ${KERNEL_OBJECTS} ${KERNEL_O_SOURCES})
add_dependencies(kernel kernel-deps)
set_target_properties(kernel PROPERTIES OUTPUT_NAME "arikoto")

# Linker flags
target_link_options(kernel PRIVATE
    -Wl,-m,elf_x86_64
    -Wl,--build-id=none
    -nostdlib
    -static
    -z max-page-size=0x1000
    -Wl,--gc-sections
    -Wl,-z,noexecstack
    -T ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
)
