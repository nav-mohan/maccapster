message("BUILDING XMLHANDLER")

add_library(xmlhandler xmlhandler.cpp)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED COMPONENTS headers)
target_include_directories(xmlhandler PUBLIC ${CMAKE_SOURCE_DIR}/libs/mslogger)
target_link_libraries(xmlhandler Boost::headers)