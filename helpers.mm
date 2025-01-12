#include <regex>
#include <Foundation/Foundation.h>

// This will be used as a base for generating other directory-paths
// This returns ./build in dev mode (unbundled executable). 
// And returns /Applications/capster.app/ in prod mode (bundled app)
std::string getBundlePath()
{
    NSString *mainBundlePath = [[NSBundle mainBundle] bundlePath];  
    return [mainBundlePath UTF8String];
}


std::string getResourcePath(const std::string& fileName) {
    NSBundle* mainBundle = [NSBundle mainBundle];
    NSString* resourcePath = [mainBundle pathForResource:@(fileName.c_str()) ofType:nil];
    std::string path = [resourcePath UTF8String];
    return path;
}


#define URL_REGEX_PATTERN   "^(https://|http://|://|//)?([\\w,.]*)(.*)"
void ParseURL(const std::string& url, std::string& host, std::string& target, std::string& port) // convert url --> host/target:port
{
    std::regex pattern(URL_REGEX_PATTERN);
    std::sregex_iterator it(url.begin(),url.end(),pattern);
    std::sregex_iterator end;

    if(it != end)
    {
        if(it->str(1) == "https://")    port = "443";
        else                            port = "80";
        host = std::move(it->str(2));
        target = std::move(it->str(3));
    }
}

void print_usage()
{
    printf("USAGE:   ./capster <MODE> <PORT> <LATITUDE> <LONGITUDE>\n");
    printf("EXAMPLE: ./capster PROD 8080 42.384 -81.836\n");
    printf("EXAMPLE: ./capster TEST 8000 42.384 -81.836\n");
}



// #include <regex>
std::regex find_hashhashhash("###");
std::regex find_hashtag("#");
std::regex find_multiwhitespace("\\s\\s+");
std::string replace_hashhashhash = "";
std::string replace_hashtag = "hastag ";
std::string repace_multiwhitespace = " ";
std::string prepareTextForTTS(const std::string ttsText)
{
    std::string modified = std::regex_replace(ttsText, find_hashhashhash, replace_hashhashhash);
    modified = std::regex_replace(modified, find_hashtag , replace_hashtag);
    modified = std::regex_replace(modified, find_multiwhitespace, repace_multiwhitespace);
    return modified;
}
