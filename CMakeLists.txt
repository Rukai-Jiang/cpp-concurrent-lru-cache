cmake_minimum_required(VERSION 3.16)
project(cpp_concurrent_lru_cache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cache_tests tests/test_cache.cpp)
target_include_directories(cache_tests PRIVATE include)

if(MSVC)
  target_compile_options(cache_tests PRIVATE /W4)
else()
  target_compile_options(cache_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME cache_tests COMMAND cache_tests)

