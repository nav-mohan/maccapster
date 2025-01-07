message("BUILDING ARCHIVEQUEUE")


if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# enable/disable required FLAGS for libarchive
include(${CMAKE_CURRENT_SOURCE_DIR}/libarchive_flags.cmake)


# Add libarchive as a subdirectory
add_subdirectory(third_party/libarchive)

# Define the archiveQueue library
add_library(archiveQueue STATIC archiveQueue.cpp zipArchiver.cpp)

# Include directories for archiveQueue and libarchive
target_include_directories(archiveQueue PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libarchive/libarchive
        ${CMAKE_SOURCE_DIR}/libs/mslogger
    )

# Link libarchive to the archiveQueue library
target_link_libraries(archiveQueue PRIVATE archive_static)


