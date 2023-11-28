message("BUILDING RS232")

add_library(rs232util rs232util.cpp)
target_include_directories(rs232util PUBLIC ${CMAKE_SOURCE_DIR}/libs/mslogger)
