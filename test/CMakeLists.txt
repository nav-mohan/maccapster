message("BUILDING SERVER")

cmake_minimum_required(VERSION 3.13)
project(CapsterServer LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add_executable(CapsterServer mockserver.mm mockserver.cpp)
add_executable(CapsterServer mockserver.cpp)

target_link_libraries(
    CapsterServer PUBLIC
    # "-framework Cocoa"
)

# # Set the source file for your executable
# # set(EXECUTABLE_SOURCE "${CMAKE_BINARY_DIR}/CapsterServer.app/Contents/MacOS/CapsterServer")
# set(EXECUTABLE_SOURCE "${CMAKE_BINARY_DIR}/CapsterServer")

# # Create the macOS application bundle
# set(BUNDLE_NAME "CapsterServer")
# set(BUNDLE_DESTINATION "${CMAKE_BINARY_DIR}/${BUNDLE_NAME}.app")
# set(CONTENTS_DIR "${BUNDLE_DESTINATION}/Contents")
# set(MACOS_DIR "${CONTENTS_DIR}/MacOS")
# set(FRAMEWORKS_DIR "${CONTENTS_DIR}/Frameworks")
# set(RESOURCES_DIR "${CONTENTS_DIR}/Resources")

# # Create the necessary directories within the bundle
# file(MAKE_DIRECTORY ${MACOS_DIR} ${FRAMEWORKS_DIR})

# # Copy your executable to the bundle
# install(PROGRAMS "${EXECUTABLE_SOURCE}" DESTINATION ${MACOS_DIR})