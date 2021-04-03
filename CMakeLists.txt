cmake_minimum_required(VERSION 3.5)

project(smooth-pwd)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Set default build type: Release")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  set(CMAKE_CXX_FLAGS "/W4 /DNDEBUG ${CMAKE_CXX_FLAGS}")
  if (NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-DNDEBUG)
  endif()
else(MSVC)
  set(CMAKE_C_FLAGS "-m64 -Wall ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-m64 -Wall ${CMAKE_CXX_FLAGS}")
endif(MSVC)

include_directories("${PROJECT_SOURCE_DIR}/src")

set(LIB_SRCS
${PROJECT_SOURCE_DIR}/src/backoff.cpp
${PROJECT_SOURCE_DIR}/src/baseTrie.cpp
${PROJECT_SOURCE_DIR}/src/kneserNey.cpp
${PROJECT_SOURCE_DIR}/src/simpleTrie.cpp
)

add_library(Markovlib STATIC ${LIB_SRCS})


#https://stackoverflow.com/questions/14306642/adding-multiple-executables-in-cmake
set(TEST_SRCS
${PROJECT_SOURCE_DIR}/example.cpp
${PROJECT_SOURCE_DIR}/guesser.cpp
)
foreach(sourcefile ${TEST_SRCS})
    file(RELATIVE_PATH filename ${PROJECT_SOURCE_DIR} ${sourcefile})
    string(REPLACE ".cpp" "" testname ${filename})
    add_executable(${testname} ${sourcefile})
    target_link_libraries(${testname} Markovlib)
endforeach(sourcefile ${TEST_SRCS})
