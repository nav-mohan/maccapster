message("BUILDING MACTTSQUEUE")
add_library(macttsqueue macttsqueue.mm)

target_link_libraries(macttsqueue "-framework Foundation -framework AVFoundation")
    
