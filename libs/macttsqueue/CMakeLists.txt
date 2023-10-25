message("BUILDING MACTTSQUEUE")
add_library(macttsqueue macttsqueue.mm)

target_include_directories(macttsqueue PUBLIC ${CMAKE_SOURCE_DIR}/libs/mslogger)
target_link_libraries(macttsqueue "-framework Foundation -framework AVFoundation")
    
