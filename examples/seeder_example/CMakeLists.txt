cmake_minimum_required(VERSION 3.16)
project(seeder_example)

include(GNUInstallDirs)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(
  SeederExample
  main.cpp
  ../common/console_logger.cpp
  ../common/create_can_driver.cpp
  vt_application.cpp
  vt_application.hpp
  seeder.cpp
  seeder.hpp
  object_pool.hpp
  section_control_implement_sim.hpp
  section_control_implement_sim.cpp)

install(TARGETS SeederExample RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/BasePool.iop"
        DESTINATION "${CMAKE_INSTALL_BINDIR}")

target_compile_features(SeederExample PUBLIC cxx_std_11)
set_target_properties(SeederExample PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  SeederExample PRIVATE isobus::Isobus isobus::HardwareIntegration
                        Threads::Threads isobus::Utility)

add_custom_command(
  TARGET SeederExample
  POST_BUILD
  COMMENT "Copying BasePool.iop to build directory"
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/BasePool.iop
          $<TARGET_FILE_DIR:SeederExample>/BasePool.iop)
