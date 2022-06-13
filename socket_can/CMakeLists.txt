cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set library public name
set(SOCKET_CAN_INTERFACE_PUBLIC_NAME "SocketCANInterface")

# Set source and include directories
set(SOCKET_CAN_SRC_DIR "src")
set(SOCKET_CAN_INCLUDE_DIR "include")

# Set source files 
set(SOCKET_CAN_SRC 
  "socket_can_interface.cpp"
)

# Prepend the source directory path to all the source files
PREPEND(SOCKET_CAN_SRC ${SOCKET_CAN_SRC_DIR} ${SOCKET_CAN_SRC})

# Set the include files
set(SOCKET_CAN_INCLUDE 
  "socket_can_interface.hpp"
)

# Prepend the include directory path to all the include files
PREPEND(SOCKET_CAN_INCLUDE ${SOCKET_CAN_INCLUDE_DIR} ${SOCKET_CAN_INCLUDE})

# Create the library from the source and include files
add_library(SocketCANInterface SHARED ${SOCKET_CAN_SRC} ${SOCKET_CAN_INCLUDE})

target_link_libraries(SocketCANInterface PRIVATE SystemTiming Isobus)

# Specify the include directory to be exported for other moduels to use. The 
# PUBLIC keyword here allows other libraries or exectuables to link to this
# library and use its functionality.
target_include_directories(SocketCANInterface PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${SOCKET_CAN_PUBLIC_NAME}/include>
 ) 
