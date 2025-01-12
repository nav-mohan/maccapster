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

// returns ./build/../ in dev mode (unbundled executable) 
// return /Applications/capster.app/Contents/ in prod mode (bundled app)
std::string getContentsDirPath()
{
    const std::string bundlePath = getBundlePath();
    const std::string suffix = ".app";
    
    // if the bundlePath has a suffix of .app (such as /Applications/capster.app) then we are executing the bundled application
    if (bundlePath.compare(bundlePath.size() - suffix.size(), suffix.size(), suffix) == 0) 
        return bundlePath + "/Contents";
    
    else // unbundled binary executable
        return bundlePath.substr(0, bundlePath.find_last_of('/'));
}

std::string getResourcePath(const std::string& fileName) {
    // There is a special method to specifically get files from the /Resources directory
    // but it points to ./build in dev-mode. 
    // and it points to /Applications/capster.app/Contents/Resources in prod-mode
    // we want it to be more consistent by always pointing to getContentsDirPath()/Resources
    // NSBundle* mainBundle = [NSBundle mainBundle];
    // NSString* resourcePath = [mainBundle pathForResource:@(fileName.c_str()) ofType:nil];
    // if (resourcePath == nil || resourcePath.length == 0) return "";
    // std::string path = [resourcePath UTF8String];
    // return path;

    return getContentsDirPath() + "/Resources/" + fileName;
}

std::string getStorageDirPath()
{
    return getContentsDirPath() + "/Storage";
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
