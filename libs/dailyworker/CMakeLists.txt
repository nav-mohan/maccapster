message("BUILDING DAILYWORKER")

add_library(dailyWorker dailyWorker.cpp)
target_include_directories(dailyWorker PUBLIC ${CMAKE_SOURCE_DIR}/libs/mslogger)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()