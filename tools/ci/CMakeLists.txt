# Standalone tests for original host code; never require an Xbox XBE or game assets.
cmake_minimum_required(VERSION 3.20)
project(cod_source_regression LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

foreach(test_name IN ITEMS damage_rules capture_path)
  add_executable(test_${test_name} ../test_${test_name}.cpp)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(test_${test_name} PRIVATE -Wall -Wextra -Werror)
  endif()
  add_test(NAME ${test_name} COMMAND test_${test_name})
endforeach()
