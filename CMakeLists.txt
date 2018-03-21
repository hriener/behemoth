cmake_minimum_required(VERSION 3.1)
project(behemoth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BEHEMOTH_EXAMPLES "Build examples" ON)

# some specific compiler definitions
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-fcolor-diagnostics" HAS_FCOLOR_DIAGNOSTICS)
if (HAS_FCOLOR_DIAGNOSTICS)
  add_definitions(-fcolor-diagnostics)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_subdirectory(lib)
add_subdirectory(include)

if(BEHEMOTH_EXAMPLES)
  add_subdirectory(examples)
endif()
