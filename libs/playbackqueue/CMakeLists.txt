message("BUILDING PLAYBACKQUEUE")
add_library(playbackqueue playbackqueue.cpp)

target_link_libraries(playbackqueue "-framework CoreFoundation -framework AudioToolbox -framework Foundation")
    
