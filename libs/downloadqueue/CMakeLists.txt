message("BUILDING DOWNLOADQUEUE")

add_library(downloadqueue downloadqueue.cpp)

find_package(Boost REQUIRED COMPONENTS headers)

# set(INTEL_OPENSSL_PATH "/usr/local/Cellar/openssl@1.1/1.1.1t")
# set(M1_OPENSSL_PATH "/opt/homebrew/Cellar/openssl@1.1/1.1.1t")
# set(OPENSSL_CMAKE_PREFIX_PATH ${INTEL_OPENSSL_PATH})
#set(OPENSSL_CMAKE_PREFIX_PATH ${M1_OPENSSL_PATH})
# set(CMAKE_PREFIX_PATH "${OPENSSL_CMAKE_PREFIX_PATH}")

find_package(OpenSSL REQUIRED)
target_link_libraries(downloadqueue Boost::headers OpenSSL::SSL OpenSSL::Crypto)

    
