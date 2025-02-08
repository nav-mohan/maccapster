#if !defined(XML_HANDLER_HPP)
#define XML_HANDLER_HPP

#include <unordered_map>
#include <functional>
#include <mutex>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>
#include "mslogger.hpp"

// this REGEX_PATTERN cannot handle array of matches "<xml><alert></alert><xml><alert></alert>"
// for the timebeing a workaround is to keep Client::BUFFER_SIZE < 1500 (size of Heartbeat)
#define REGEX_PATTERN "<\\?xml\\s.*?<\\/alert>"

struct HistoryItem
{
    std::string identifier_;
    std::string eventType_;
    std::time_t timestamp_;
    std::string language_;
    
    HistoryItem(std::string identifier, std::string eventType, std::string timestamp, std::string language)
    : identifier_(identifier), eventType_(eventType), language_(language) {
        auto tm = parseTimestamp(timestamp);
        std::time_t t1 = std::mktime(&tm);
        timestamp_ = t1;
    }

    // checks if two alerts are identical, or the same event that happened within 15 minutes of each other
    const bool is_basically_same(const HistoryItem& other) const {
        if(language_ != other.language_) return false;
        if (identifier_ == other.identifier_) return true;
        if(eventType_ != other.eventType_) return false;
        const double difference = std::difftime(timestamp_, other.timestamp_);
        if (std::abs<double>(difference) > 900) return false;
        return true;
    }
    
    // convert string timestamp to std::tm object
    std::tm parseTimestamp(const std::string& timestamp) {
        std::tm tm = {};
        std::istringstream ss(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        return tm;
    }
};

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
    std::function<void(std::string content, std::string filename)> writeToFile;

private:
    boost::regex regexPattern_;
    void find_match(std::string& host);
    std::unordered_map<std::string,std::string> fullBuffer_;
    std::queue<std::string> matches_;
    void process_matches(std::string& host);
    inline static std::vector<const HistoryItem> history_ = {};
    inline static std::mutex mut_;
    inline static bool check_update_history(const HistoryItem historyItem)
    {
        basic_log("CHECKING HISTORY " + historyItem.identifier_, INFO);
        std::lock_guard<std::mutex> lk(XmlHandler::mut_);
        for(auto it = history_.rbegin(); it != history_.rend(); it++)
            if(historyItem.is_basically_same(*it))
                return true;
        XmlHandler::history_.push_back(historyItem);
        return false;
    }
};

#endif // XML_HANDLER_HPP