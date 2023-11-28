message("BUILDING GEOUTIL")

add_library(geoutil geoutil.cpp)
target_include_directories(geoutil PUBLIC ${CMAKE_SOURCE_DIR}/libs/mslogger)
