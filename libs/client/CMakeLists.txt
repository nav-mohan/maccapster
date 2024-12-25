message("BUILDING CLIENT")

add_library(client client.cpp)
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

find_package(Boost REQUIRED COMPONENTS headers)
target_include_directories(client PUBLIC ${CMAKE_SOURCE_DIR}/libs/mslogger)
target_link_libraries(client Boost::headers)