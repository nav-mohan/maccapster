#if !defined(XML_HANDLER_HPP)
#define XML_HANDLER_HPP

#include <unordered_map>
#include <functional>
#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>
#include "mslogger.hpp"

// this REGEX_PATTERN cannot handle array of matches "<xml><alert></alert><xml><alert></alert>"
// for the timebeing a workaround is to keep Client::BUFFER_SIZE < 1500 (size of Heartbeat)
#define REGEX_PATTERN "<\\?xml\\s.*?<\\/alert>"

class XmlHandler
{
public:
    XmlHandler() : regexPattern_(REGEX_PATTERN){}; 
    ~XmlHandler(){};
    void append(std::string& host, const std::vector<char>&& tempBuffer,size_t size);
    // static void clear_history(){if(history_.size()) history_.clear();} 
    std::function<int(const std::string& filename)> playAudioFile;
    std::function<bool(std::string&& boundary)> checkArea;
    std::function<void(std::string&& encodedData,const std::string& filename)> decodeToFile;
    std::function<void(const std::string& uri, const std::string& filename)> enqueueDwnld;
    std::function<void(std::string&& text, std::string& filename, const std::string& language)> enqueueTTS;

private:
    boost::regex regexPattern_;
    void find_match(std::string& host);
    std::unordered_map<std::string,std::string> fullBuffer_;
    std::queue<std::string> matches_;
    void process_matches(std::string& host);
    inline static std::vector<std::string> history_ = {};
    inline static std::mutex mut_;
    inline static bool check_update_history(std::string h)
    {
        
        basic_log("CHECKING HISTORY " + h,INFO);
        std::lock_guard<std::mutex> lk(XmlHandler::mut_);
        for(auto it = history_.rbegin(); it != history_.rend(); it++)
            if(h == *it)
                return true;
        XmlHandler::history_.push_back(h);
        return false;
    }
};

#endif // XML_HANDLER_HPP