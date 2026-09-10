# ==============================================================================
# CMake toolchain file for ARM none EABI cross-compilation Supports GCC and
# Clang compilers with various ARM processors
# ==============================================================================
#
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=ArmNoneEabiToolchain.cmake \
# -DARM_TOOLCHAIN_PATH=/path/to/toolchain \ -DARM_CPU_ARCH=cortex-m7-neon \
# -DCMAKE_BUILD_TYPE=RelWithDebInfo \ -S . -B build
#
# Requirements: - ARM_TOOLCHAIN_PATH must be set (environment variable or CMake
# variable) - ARM_CPU_ARCH must specify the target CPU architecture
# ==============================================================================

cmake_minimum_required(VERSION 3.16)

if(DEFINED ARM_TOOLCHAIN_INITIALIZED)
  return()
endif()

# ------------------------------------------------------------------------------
# REQUIREMENTS CHECK
# ------------------------------------------------------------------------------
# Check if user is trying to set ARM_NONE_EABI_PLATFORM externally
if(DEFINED ARM_NONE_EABI_PLATFORM)
  message(
    FATAL_ERROR
      "ARM_NONE_EABI_PLATFORM cannot be set manually.\n"
      "This variable is set automatically by the toolchain file.\n"
      "Remove -DARM_NONE_EABI_PLATFORM from your CMake command line.")
endif()

# Now we can set it internally
set(ARM_NONE_EABI_PLATFORM
    TRUE
    CACHE INTERNAL
          "ARM none EABI platform is being used (set by toolchain file)")

# Mark ARM_NONE_EABI_PLATFORM as read-only to prevent external modification
set_property(CACHE ARM_NONE_EABI_PLATFORM PROPERTY TYPE INTERNAL)

if(NOT DEFINED ARM_TOOLCHAIN_PATH)
  if(DEFINED ENV{ARM_TOOLCHAIN_PATH})
    set(ARM_TOOLCHAIN_PATH "$ENV{ARM_TOOLCHAIN_PATH}")
    message(
      STATUS "Using ARM_TOOLCHAIN_PATH from environment: ${ARM_TOOLCHAIN_PATH}")
  else()
    message(
      FATAL_ERROR
        "ARM_TOOLCHAIN_PATH is not defined!\n"
        "Set it as environment variable or pass -DARM_TOOLCHAIN_PATH=/path/to/toolchain\n"
        "Example: export ARM_TOOLCHAIN_PATH=/opt/arm-toolchain")
  endif()
endif()

# Cache the variable for persistence
set(ARM_TOOLCHAIN_PATH
    "${ARM_TOOLCHAIN_PATH}"
    CACHE PATH "Path to ARM toolchain directory (containing bin/, lib/, etc.)")

# Check toolchain path exists
if(NOT EXISTS "${ARM_TOOLCHAIN_PATH}")
  message(
    FATAL_ERROR "ARM toolchain directory not found: ${ARM_TOOLCHAIN_PATH}\n"
                "Please check the path and try again.")
endif()

# ------------------------------------------------------------------------------
# SYSTEM CONFIGURATION
# ------------------------------------------------------------------------------
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CROSSCOMPILING TRUE)

# # Tell CMake to test compiler with these flags
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Pass toolchain variables to try_compile projects
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES ARM_TOOLCHAIN_PATH ARM_CPU_ARCH
                                         CMAKE_C_COMPILER CMAKE_CXX_COMPILER)

# ------------------------------------------------------------------------------
# COMPILER DETECTION AND SETUP
# ------------------------------------------------------------------------------
# Try to detect available compilers in the toolchain
function(detect_compilers toolchain_path)
  # If compilers are already set (e.g., from command line), use them
  if(DEFINED CMAKE_C_COMPILER AND CMAKE_C_COMPILER)
    message(STATUS "Using pre-defined C compiler: ${CMAKE_C_COMPILER}")
  else()
    set(possible_compilers)
    # List of possible compiler names
    set(c_compiler_names clang arm-none-eabi-gcc gcc)

    # Find C compiler
    foreach(comp ${c_compiler_names})
      set(full_path "${toolchain_path}/bin/${comp}")
      if(EXISTS "${full_path}")
        set(CMAKE_C_COMPILER
            "${full_path}"
            PARENT_SCOPE)
        set(CMAKE_C_COMPILER "${full_path}")
        message(STATUS "Found C compiler: ${full_path}")
        break()
      endif()
    endforeach()

    # Fallback: if not found, use generic names and hope they're in PATH
    if(NOT CMAKE_C_COMPILER)
      set(CMAKE_C_COMPILER
          "clang"
          PARENT_SCOPE)
      message(
        WARNING "C compiler not found in toolchain, using 'clang' from PATH")
    endif()
  endif()

  if(DEFINED CMAKE_CXX_COMPILER AND CMAKE_CXX_COMPILER)
    message(STATUS "Using pre-defined C++ compiler: ${CMAKE_CXX_COMPILER}")
  else()
    # List of possible compiler names
    set(cxx_compiler_names clang++ arm-none-eabi-g++ g++)

    # Find C++ compiler
    foreach(comp ${cxx_compiler_names})
      set(full_path "${toolchain_path}/bin/${comp}")
      if(EXISTS "${full_path}")
        set(CMAKE_CXX_COMPILER
            "${full_path}"
            PARENT_SCOPE)
        set(CMAKE_CXX_COMPILER "${full_path}")
        message(STATUS "Found C++ compiler: ${full_path}")
        break()
      endif()
    endforeach()

    # Fallback: if not found, use generic names and hope they're in PATH
    if(NOT CMAKE_CXX_COMPILER)
      set(CMAKE_CXX_COMPILER
          "clang++"
          PARENT_SCOPE)
      message(
        WARNING "C++ compiler not found in toolchain, using 'clang++' from PATH"
      )
    endif()
  endif()
endfunction()

# Detect and set compilers
detect_compilers(${ARM_TOOLCHAIN_PATH})

# Set compiler targets for Clang compatibility
set(CMAKE_C_COMPILER_TARGET arm-none-eabi)
set(CMAKE_CXX_COMPILER_TARGET arm-none-eabi)
set(CMAKE_ASM_COMPILER_TARGET arm-none-eabi)

# ------------------------------------------------------------------------------
# COMPILER TYPE DETECTION AND FLAGS
# ------------------------------------------------------------------------------
# Check compiler type AFTER it's been set
if(CMAKE_C_COMPILER)
  # Run a test to determine compiler type
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --version
    OUTPUT_VARIABLE COMPILER_VERSION_OUTPUT
    ERROR_VARIABLE COMPILER_VERSION_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  message(STATUS "Compiler info: ${COMPILER_VERSION_OUTPUT}")

  # Detect compiler type
  if("${COMPILER_VERSION_OUTPUT}" MATCHES "clang"
     OR "${COMPILER_VERSION_OUTPUT}" MATCHES "LLVM")
    set(COMPILER_TYPE "CLANG")
    message(STATUS "Compiler detected: Clang/LLVM")
  elseif("${COMPILER_VERSION_OUTPUT}" MATCHES "GCC"
         OR "${COMPILER_VERSION_OUTPUT}" MATCHES "gcc")
    set(COMPILER_TYPE "GCC")
    message(STATUS "Compiler detected: GCC")
  else()
    set(COMPILER_TYPE "UNKNOWN")
    message(WARNING "Unknown compiler type, assuming Clang for ARM")
  endif()
else()
  message(WARNING "C compiler not set, assuming Clang for ARM")
  set(COMPILER_TYPE "CLANG")
endif()

# ------------------------------------------------------------------------------
# CPU ARCHITECTURE CONFIGURATION
# ------------------------------------------------------------------------------
if(NOT ARM_CPU_ARCH)
  set(ARM_CPU_ARCH
      cortex-m4
      CACHE
        STRING
        "ARM CPU/architecture (e.g., cortex-m0, cortex-m3, cortex-m4, cortex-m4-fpu, cortex-m7, cortex-m7-neon)"
  )
  message(STATUS "ARM_CPU_ARCH not specified, using default: ${ARM_CPU_ARCH}")
endif()

# Parse ARM_CPU_ARCH to extract CPU, architecture, FPU and float ABI settings
set(CPU_CONFIG_DEFINED FALSE)

# cortex-m0 series
if(ARM_CPU_ARCH MATCHES "^cortex-m0")
  set(ARM_CPU "cortex-m0")
  set(ARCH "armv6-m")
  set(CPU_CONFIG_DEFINED TRUE)

  if(ARM_CPU_ARCH STREQUAL "cortex-m0plus")
    set(ARM_CPU "cortex-m0plus")
  endif()

  # cortex-m3 series
elseif(ARM_CPU_ARCH STREQUAL "cortex-m3")
  set(ARM_CPU "cortex-m3")
  set(ARCH "armv7-m")
  set(CPU_CONFIG_DEFINED TRUE)

  # cortex-m4 series
elseif(ARM_CPU_ARCH MATCHES "^cortex-m4")
  set(ARM_CPU "cortex-m4")
  set(ARCH "armv7e-m")
  set(CPU_CONFIG_DEFINED TRUE)

  # cortex-m7 series
elseif(ARM_CPU_ARCH MATCHES "^cortex-m7")
  set(ARM_CPU "cortex-m7")
  set(ARCH "armv7e-m")
  set(CPU_CONFIG_DEFINED TRUE)
endif()

# Set FPU and float ABI based on CPU type and suffix
if(CPU_CONFIG_DEFINED)
  # Default: no FPU, soft float
  set(ARM_FPU "")
  set(ARM_FLOAT_ABI "soft")

  # Check for FPU variants
  if(ARM_CPU_ARCH MATCHES ".*-fpu$" OR ARM_CPU_ARCH MATCHES ".*-neon$")
    set(ARM_FLOAT_ABI "hard")

    if(ARM_CPU_ARCH MATCHES "cortex-m4-fpu")
      set(ARM_FPU "fpv4-sp-d16")
    elseif(ARM_CPU_ARCH MATCHES "cortex-m7-fpv5")
      set(ARM_FPU "fpv5-d16")
    elseif(ARM_CPU_ARCH MATCHES "cortex-m7-neon")
      set(ARM_FPU "neon")
    elseif(ARM_CPU STREQUAL "cortex-m7")
      # Default FPU for cortex-m7
      set(ARM_FPU "fpv5-d16")
      set(ARM_FLOAT_ABI "hard")
    endif()
  elseif(ARM_CPU STREQUAL "cortex-m7")
    # cortex-m7 always has FPU
    set(ARM_FPU "fpv5-d16")
    set(ARM_FLOAT_ABI "hard")
  endif()
else()
  message(
    WARNING
      "Unknown ARM_CPU_ARCH '${ARM_CPU_ARCH}', using cortex-m4 with FPU as default"
  )
  set(ARM_CPU "cortex-m4")
  set(ARCH "armv7e-m")
  set(ARM_FPU "fpv4-sp-d16")
  set(ARM_FLOAT_ABI "hard")
endif()

# ------------------------------------------------------------------------------
# COMPILER FLAGS CONSTRUCTION
# ------------------------------------------------------------------------------

# Set target for Clang based on architecture
if(COMPILER_TYPE STREQUAL "CLANG")
  list(APPEND ARM_COMMON_FLAGS "--target=arm-none-eabi")
  list(APPEND ARM_COMMON_FLAGS "-stdlib=libc++")
endif()

# Add GCC-specific flags
if(COMPILER_TYPE STREQUAL "GCC")
  list(APPEND ARM_COMMON_FLAGS -specs=nosys.specs)
endif()

# Add architecture flags
if(ARM_CPU)
  list(APPEND ARM_COMMON_FLAGS -mcpu=${ARM_CPU})
endif()

if(ARCH)
  list(APPEND ARM_COMMON_FLAGS -march=${ARCH})
endif()

# Add FPU flags if specified
if(ARM_FPU)
  list(APPEND ARM_COMMON_FLAGS -mfpu=${ARM_FPU})
endif()

# Add float ABI flags
if(ARM_FLOAT_ABI)
  list(APPEND ARM_COMMON_FLAGS -mfloat-abi=${ARM_FLOAT_ABI})
endif()

# Common flags for all compilers
list(
  APPEND
  ARM_COMMON_FLAGS
  -mthumb
  -ffunction-sections
  -fdata-sections
  -fno-rtti
  -fno-exceptions
  -fno-threadsafe-statics)

# Convert list to string for CMAKE flags
string(REPLACE ";" " " ARM_COMPILE_FLAGS_STR "${ARM_COMMON_FLAGS}")

# ------------------------------------------------------------------------------
# CMake FLAGS SETUP
# ------------------------------------------------------------------------------
# Set compiler flags for try_compile tests
set(CMAKE_C_FLAGS_INIT "${ARM_COMPILE_FLAGS_STR}")
set(CMAKE_CXX_FLAGS_INIT "${ARM_COMPILE_FLAGS_STR}")
set(CMAKE_ASM_FLAGS_INIT "${ARM_COMPILE_FLAGS_STR}")

# Set actual compiler flags (can be overridden by project)
set(CMAKE_C_FLAGS
    "${ARM_COMPILE_FLAGS_STR}"
    CACHE STRING "C compiler flags for ARM")
set(CMAKE_CXX_FLAGS
    "${ARM_COMPILE_FLAGS_STR}"
    CACHE STRING "C++ compiler flags for ARM")
set(CMAKE_ASM_FLAGS
    "${ARM_COMPILE_FLAGS_STR}"
    CACHE STRING "Assembly compiler flags for ARM")

# ------------------------------------------------------------------------------
# 1. BUILD TYPE CONFIGURATION
# ------------------------------------------------------------------------------
# Common flags for all build types
set(COMMON_DEBUG_FLAGS "-g3")
set(COMMON_RELEASE_FLAGS "")

# Debug build flags
set(CMAKE_C_FLAGS_DEBUG
    "${COMMON_DEBUG_FLAGS} -O0 -DDEBUG"
    CACHE STRING "Debug C flags for ARM" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG
    "${COMMON_DEBUG_FLAGS} -O0 -DDEBUG"
    CACHE STRING "Debug C++ flags for ARM" FORCE)

# Release build flags
set(CMAKE_C_FLAGS_RELEASE
    "${COMMON_RELEASE_FLAGS} -O3 -DNDEBUG"
    CACHE STRING "Release C flags for ARM" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
    "${COMMON_RELEASE_FLAGS} -O3 -DNDEBUG"
    CACHE STRING "Release C++ flags for ARM" FORCE)

# MinSizeRel build flags
set(CMAKE_C_FLAGS_MINSIZEREL
    "${COMMON_RELEASE_FLAGS} -Os -DNDEBUG"
    CACHE STRING "MinSizeRel C flags for ARM" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL
    "${COMMON_RELEASE_FLAGS} -Os -DNDEBUG"
    CACHE STRING "MinSizeRel C++ flags for ARM" FORCE)

# RelWithDebInfo build flags
set(CMAKE_C_FLAGS_RELWITHDEBINFO
    "${COMMON_DEBUG_FLAGS} -O2 -DNDEBUG"
    CACHE STRING "RelWithDebInfo C flags for ARM" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
    "${COMMON_DEBUG_FLAGS} -O2 -DNDEBUG"
    CACHE STRING "RelWithDebInfo C++ flags for ARM" FORCE)

# ------------------------------------------------------------------------------
# SYSROOT AND LIBRARY SETUP
# ------------------------------------------------------------------------------

if(COMPILER_TYPE STREQUAL "GCC")
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-sysroot
    OUTPUT_VARIABLE COMPILER_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(COMPILER_SYSROOT)
    set(CMAKE_SYSROOT "${COMPILER_SYSROOT}")
    message(
      STATUS "Using sysroot from pre-installed GCC compiler: ${CMAKE_SYSROOT}")
  endif()
elseif(COMPILER_TYPE STREQUAL "CLANG")
  # For Clang, compute sysroot as parent directory of compiler's bin directory
  get_filename_component(COMPILER_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
  get_filename_component(COMPILER_ROOT "${COMPILER_BIN_DIR}" DIRECTORY)
  if(EXISTS "${COMPILER_ROOT}/lib/clang-runtimes")
    # Get list of runtime directories
    file(
      GLOB RUNTIME_DIRS
      LIST_DIRECTORIES true
      "${COMPILER_ROOT}/lib/clang-runtimes/*")

    # Prepare ARM_FPU for comparison by replacing '-' with '_'
    set(ARM_FPU_COMPARE "${ARM_FPU}")
    if(ARM_FPU)
      string(REPLACE "-" "_" ARM_FPU_COMPARE "${ARM_FPU}")
    endif()

    # Find the most suitable directory based on matching parameters
    set(SELECTED_MULTILIB_DIR "")
    set(MAX_MATCHES 0)

    foreach(dir ${RUNTIME_DIRS})
      get_filename_component(dir_name ${dir} NAME)

      set(matches 0)
      # Check ARCH match
      if(dir_name MATCHES "${ARCH}")
        math(EXPR matches "${matches} + 1")
      endif()
      # Check ARM_FLOAT_ABI match
      if(ARM_FLOAT_ABI AND dir_name MATCHES "${ARM_FLOAT_ABI}")
        math(EXPR matches "${matches} + 1")
      endif()
      # Check ARM_FPU match (after replacement)
      if(ARM_FPU_COMPARE AND dir_name MATCHES "${ARM_FPU_COMPARE}")
        math(EXPR matches "${matches} + 1")
      endif()

      # Select directory with highest number of matches
      if(matches GREATER MAX_MATCHES)
        set(MAX_MATCHES ${matches})
        set(SELECTED_MULTILIB_DIR ${dir})
      endif()
    endforeach()

    # If no matches at all, use the first available directory
    if(NOT SELECTED_MULTILIB_DIR AND RUNTIME_DIRS)
      list(GET RUNTIME_DIRS 0 SELECTED_MULTILIB_DIR)
    endif()

    # Set sysroot to selected multilib directory
    if(SELECTED_MULTILIB_DIR)
      set(CMAKE_SYSROOT "${SELECTED_MULTILIB_DIR}")
    else()
      set(CMAKE_SYSROOT "${COMPILER_ROOT}/lib/clang-runtimes")
    endif()
    message(
      STATUS
        "Using sysroot from pre-installed Clang compiler path: ${CMAKE_SYSROOT}"
    )
  endif()
endif()

# Set find root path to sysroot if defined
if(DEFINED CMAKE_SYSROOT)
  set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT}")
  set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

# ------------------------------------------------------------------------------
# CONFIGURATION SUMMARY
# ------------------------------------------------------------------------------
message(STATUS "========================================")
message(STATUS "ARM none EABI toolchain configuration")
message(STATUS "========================================")
message(STATUS "Toolchain path: ${ARM_TOOLCHAIN_PATH}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler type: ${COMPILER_TYPE}")
message(STATUS "Target triple: arm-none-eabi")
message(STATUS "CPU architecture: ${ARM_CPU_ARCH}")
message(STATUS "CPU: ${ARM_CPU}")
message(STATUS "Architecture: ${ARCH}")
message(STATUS "FPU: ${ARM_FPU}")
message(STATUS "Float ABI: ${ARM_FLOAT_ABI}")
message(STATUS "Common flags: ${ARM_COMPILE_FLAGS_STR}")
message(
  STATUS "ARM_NONE_EABI_PLATFORM: ${ARM_NONE_EABI_PLATFORM} (set by toolchain)")
message(STATUS "========================================")

set(ARM_TOOLCHAIN_INITIALIZED
    TRUE
    CACHE INTERNAL "")
