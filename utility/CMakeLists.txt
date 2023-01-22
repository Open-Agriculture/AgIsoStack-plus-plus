cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set source and include directories
set(UTILITY_SRC_DIR "src")
set(UTILITY_INCLUDE_DIR "include/isobus/utility")

# Set source files
set(UTILITY_SRC "system_timing.cpp" "processing_flags.cpp"
                "iop_file_interface.cpp")

# Prepend the source directory path to all the source files
prepend(UTILITY_SRC ${UTILITY_SRC_DIR} ${UTILITY_SRC})

# Set the include files
set(UTILITY_INCLUDE "system_timing.hpp" "processing_flags.hpp"
                    "iop_file_interface.hpp" "to_string.hpp")

# Prepend the include directory path to all the include files
prepend(UTILITY_INCLUDE ${UTILITY_INCLUDE_DIR} ${UTILITY_INCLUDE})

# Create the library from the source and include files
add_library(Utility ${UTILITY_SRC} ${UTILITY_INCLUDE})
add_library(${PROJECT_NAME}::Utility ALIAS Utility)

# Specify the include directory to be exported for other moduels to use. The
# PUBLIC keyword here allows other libraries or exectuables to link to this
# library and use its functionality.
target_include_directories(
  Utility PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                 $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(
  TARGETS Utility
  EXPORT IsobusTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)
