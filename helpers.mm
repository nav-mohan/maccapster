#include <regex>
#include <ctime>
#include <chrono>
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

/**
    This was an earlier implementation of getResourcePath(filename)
    It used a method NSBundle::pathForResource to get files from the /Resources directory
    but it points to ./build in dev-mode. and /Applications/capster.app/Contents/Resources in prod-mode
    std::string getResourcePath(const std::string& fileName) {
        NSBundle* mainBundle = [NSBundle mainBundle];
        NSString* resourcePath = [mainBundle pathForResource:@(fileName.c_str()) ofType:nil];
        if (resourcePath == nil || resourcePath.length == 0) return "";
        std::string path = [resourcePath UTF8String];
        return path;
    }
*/

// This returns ./build/../Resources in dev-mode
// and /Applications/capster.app/Contents/Resources in prod mode
std::string getResourcePath(const std::string& fileName) 
{
    return getContentsDirPath() + "/Resources/" + fileName;
}

// This returns ./build/../Storage in dev-mode
// and /Applications/capster.app/Contents/Storage in prod mode
std::string getStorageDirPath()
{
    return getContentsDirPath() + "/Storage";
}

// returns today's date "2024-05-31"
std::string getTodayDate() {
    // Get the current time
    auto now = std::chrono::system_clock::now();
    // Convert to time_t
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    // Convert to tm structure
    std::tm localTime = *std::localtime(&currentTime);

    // Format the date as YYYY-MM-DD
    std::ostringstream dateStream;
    dateStream << std::put_time(&localTime, "%Y-%m-%d");
    return dateStream.str();
}

// returns ./build/../Storage/2024-09-11 in dev-mode
// and /Applications/capster.app/Contents/Storage/2024-09-11 in prod-mode
std::string getDailyDirPath()
{
    std::string path = getStorageDirPath();
    std::string date = getTodayDate();
    return path + "/" + date;
}

bool moveFile(const std::string& srcPath, const std::string& dstPath)
{
    // Define source and destination file paths
    const std::filesystem::path src = srcPath;
    const std::filesystem::path dst = dstPath;
    const std::filesystem::path dstDir = dst.parent_path();

    std::cout << src << " " << dst << " " << dstDir << std::endl;

    // Check if destination directory exists
    if(!std::filesystem::exists(dstDir))
        std::filesystem::create_directories(dstDir);

    // Check if source file exists
    if (!std::filesystem::exists(src)) 
    {
        std::cerr << "Error: Source file does not exist.\n";
        return false;
    }

    // Check if the destination file already exists
    if (std::filesystem::exists(dst)) 
    {
        std::cerr << "Error: Destination file already exists.\n";
        return false;
    }

    try 
    {
        std::filesystem::rename(src, dst);
        std::cout << "File moved successfully from " << src << " to " << dst << ".\n";
    }
    catch (const std::filesystem::filesystem_error& e) 
    {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        return false;
    }
    return true;    
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
