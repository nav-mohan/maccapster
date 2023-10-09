message("BUILDING CAPSTER")

cmake_minimum_required(VERSION 3.13)
project(Capster LANGUAGES CXX C)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TARGET_NAME capster)
add_executable(${TARGET_NAME} main.mm gui.mm)
# add_compile_options(-O0 -g)

add_subdirectory(libs/client)
add_subdirectory(libs/xmlhandler)
add_subdirectory(libs/decodeutil)
add_subdirectory(libs/downloadqueue)
add_subdirectory(libs/rs232util)
add_subdirectory(libs/geoutil)
add_subdirectory(libs/macttsqueue)
add_subdirectory(libs/playbackqueue)

target_include_directories( ${TARGET_NAME} PUBLIC 
    libs/client         libs/xmlhandler
    libs/decodeutil     libs/downloadqueue
    libs/rs232util      libs/geoutil
    libs/macttsqueue    libs/playbackqueue
    )

target_link_libraries(${TARGET_NAME} PUBLIC 
    client xmlhandler decodeutil 
    downloadqueue rs232util 
    geoutil macttsqueue playbackqueue
    "-framework Cocoa"
)



# FOR BUILDING A MacOS Application

# Set the source file for your executable
set(EXECUTABLE_SOURCE "${CMAKE_BINARY_DIR}/capster")

# Create the macOS application bundle
set(BUNDLE_NAME "capster")
set(BUNDLE_DESTINATION "${CMAKE_BINARY_DIR}/${BUNDLE_NAME}.app")
set(CONTENTS_DIR "${BUNDLE_DESTINATION}/Contents")
set(MACOS_DIR "${CONTENTS_DIR}/MacOS")
set(FRAMEWORKS_DIR "${CONTENTS_DIR}/Frameworks")
set(RESOURCES_DIR "${CONTENTS_DIR}/Resources")

# Create the necessary directories within the bundle
file(MAKE_DIRECTORY ${MACOS_DIR} ${FRAMEWORKS_DIR} ${RESOURCES_DIR})

# Copy your executable to the bundle
install(PROGRAMS "${EXECUTABLE_SOURCE}" DESTINATION ${MACOS_DIR})

# set the path to resources
set(RESOURCE_FILES 
    "${CMAKE_BINARY_DIR}/../Resources/siren.wav" 
    "${CMAKE_BINARY_DIR}/../Resources/cacert.pem"
)
foreach(RES_FILE ${RESOURCE_FILES})
    install(FILES ${RES_FILE} DESTINATION ${RESOURCES_DIR})
endforeach(RES_FILE ${RESOURCE_FILES})


# Set the path to your dynamic libraries
set(DYNAMIC_LIBRARIES 
    "/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib" 
    "/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib"
    # "/usr/local/opt/onnxruntime/lib/libonnxruntime.1.14.1.dylib"
)
set_target_properties(${TARGET_NAME} PROPERTIES
    INSTALL_RPATH "@executable_path/../Resources")

# Copy the dynamic libraries to the bundle's Frameworks directory
foreach(LIBRARY ${DYNAMIC_LIBRARIES})
    install(FILES ${LIBRARY} DESTINATION ${FRAMEWORKS_DIR})
endforeach()

# execute the capster.app sitting in your build folder. either by double-clicking or by executing the binary within
# before you package using ASMALONEY's script
# cd capster.app/Contents/MacOS/
# install_name_tool -change "/opt/homebrew/opt/openssl@3/lib/libssl.3.dylib" "/Applications/capster.app/Contents/Frameworks/libssl.3.dylib" capster
# install_name_tool -change "/opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib" "/Applications/capster.app/Contents/Frameworks/libcrypto.3.dylib" capster

