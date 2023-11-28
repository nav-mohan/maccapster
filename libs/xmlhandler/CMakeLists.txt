message("BUILDING XMLHANDLER")

add_library(xmlhandler xmlhandler.cpp)

find_package(Boost REQUIRED COMPONENTS headers)
target_include_directories(xmlhandler PUBLIC ${CMAKE_SOURCE_DIR}/libs/mslogger)
target_link_libraries(xmlhandler Boost::headers)