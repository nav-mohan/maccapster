#include <string>
#include <vector>

#define PROD_SERVER1 "streaming1.naad-adna.pelmorex.com" 
#define PROD_SERVER2 "streaming2.naad-adna.pelmorex.com" 
#define PROD_PORT "8080"

#define LOCAL_TEST_SERVER1 "0.0.0.0" 
#define LOCAL_TEST_SERVER2 "127.0.0.1" 
#define LOCAL_TEST_PORT "8080"

#define REMOTE_TEST_SERVER1 "167.99.183.228" 
#define REMOTE_TEST_SERVER2 "167.99.183.228" 
#define REMOTE_TEST_PORT "8080"

#define LONDON_LAT "42.384"
#define LONDON_LON "-81.836"

#define STORAGE_LOCATION "/home/Users/nav/Downloads/audiofiles"

// keep this a little above the server's heartbeat (60secs)
// if you make this too close to 60 or a divisor of 60 (eg 15) you might end up perfectly missing the heartbeat
#define HEARTBEAT_FREQUENCY "65" 

struct UserSettings
{
    std::string server1_        = PROD_SERVER1;
    std::string port1_          = PROD_PORT;
    std::string server2_        = PROD_SERVER2;
    std::string port2_          = PROD_PORT;
    std::string latitude_       = LONDON_LAT;
    std::string longitude_      = LONDON_LON;
    std::string heartbeatFreq_  = HEARTBEAT_FREQUENCY;
    std::string storageLocation_= STORAGE_LOCATION;
};