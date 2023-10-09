#include <regex>
#include <Foundation/Foundation.h>

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

