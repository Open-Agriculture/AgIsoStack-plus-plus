cmake_minimum_required(VERSION 3.16)

function(check_std_thread_support VARIABLE)
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

  file(
    WRITE "${CMAKE_CURRENT_BINARY_DIR}/thread_test.cpp"
    "#include <thread>\nint main() {\n    std::thread t([](){});\n    t.join();\n    return 0;\n}"
  )

  try_compile(
    ${VARIABLE} "${CMAKE_BINARY_DIR}/thread_check_build"
    SOURCES "${CMAKE_CURRENT_BINARY_DIR}/thread_test.cpp"
    CMAKE_FLAGS -DCMAKE_CXX_STANDARD=11 -DCMAKE_CXX_STANDARD_REQUIRED=ON)
endfunction()
