#include "xmlhandler.hpp"

void XmlHandler::append(std::string &host, const std::vector<char> &&tempBuffer, size_t size)
{
    fullBuffer_[host].insert(fullBuffer_[host].end(), tempBuffer.begin(), tempBuffer.begin() + size);
    find_match(host);
    process_matches(host);
}

void XmlHandler::find_match(std::string &host)
{
    boost::sregex_iterator iter(fullBuffer_[host].begin(), fullBuffer_[host].end(), regexPattern_);
    boost::sregex_iterator end;
    ptrdiff_t prevLength = 0;
    ptrdiff_t prevPos = 0;
    ptrdiff_t curPos = 0;
    while (iter != end)
    {
        std::cout << "iter_pos = " << iter->position() << std::endl;
        std::cout << "iter_len = " << iter->length() << std::endl;
        curPos = iter->position() - prevPos - prevLength;
        std::cout << curPos << "," << prevPos << "," << prevLength << std::endl;
        matches_.emplace(fullBuffer_[host].substr(curPos, iter->length()));

        // std::cout << "\n++++++";
        // for(int i = 0; i < 10; i++) std::cout << matches_.back()[i];
        // std::cout << " ... ";    
        // for(int i = 10; i >= 1; i--) std::cout << matches_.back()[matches_.back().size()-i];
        // std::cout << "---------\n";

        fullBuffer_[host].erase(curPos, iter->length());
        boost::algorithm::trim(fullBuffer_[host]); // remove leading and trailing whitespaces
        std::cout << "LEFTOVER " << fullBuffer_[host].size() << "\n";
        prevLength = iter->length();
        prevPos = iter->position();
        iter++;
    }
}

void XmlHandler::process_matches(std::string &host)
{
    if (matches_.empty()) return;
    std::string y(std::move(matches_.front()));
    matches_.pop();
    std::stringstream ss(y);
    boost::property_tree::ptree tree;
    try
    {
        boost::property_tree::read_xml(ss, tree);
    }
    catch (const std::exception &e)
    {
        std::cout << "################# " << e.what() << " #################" << std::endl;
        std::cout << y << std::endl;
        std::cout << "#############################################" << std::endl;
        return;
    }
    boost::property_tree::ptree alert = tree.get_child("alert");
    boost::property_tree::ptree identifier = alert.get_child("identifier");
    boost::property_tree::ptree sender = alert.get_child("sender");
    boost::property_tree::ptree sent = alert.get_child("sent");
    boost::property_tree::ptree status = alert.get_child("status");
    boost::property_tree::ptree msgType = alert.get_child("msgType");
    boost::property_tree::ptree scope = alert.get_child("scope");
    
    // Heartbeat
    if (status.data() == "System") return;

    // Not Actual?
    if (status.data() != "Actual")
    {
        std::cout << host << " received probably test " << status.data() << std::endl;
        return;
    }

    // Update
    // if(msgType.data() == "Update") return;
    
    // std::cout << "RECEVIED ACTUAL DATA\n";

    // check history
    if (XmlHandler::check_update_history(identifier.data())) return;

    std::vector<boost::property_tree::ptree::iterator> infoNodes;
    for (auto it = alert.begin(); it != alert.end(); ++it)
    {
        if (it->first == "info")
            infoNodes.push_back(it);
    }
    if (infoNodes.size() == 0) // No <info> nodes
    {
        std::cout << "NO INFO NODES\n";
        return;
    }

    for (auto info : infoNodes)
    {
        std::string resourceDesc;
        std::string mimeType;
        std::string uri;
        std::string size;
        std::string encodedData;
        std::string language;
        std::string urgency;
        std::string severity;
        bool areaCovered = 1; // set this back to zero after you're done debugging...
        std::string headline;
        std::string description;
        std::string instruction;

        language = "en-US"; // default language
        if (info->second.find("language") != info->second.not_found())
        {
            language = info->second.get_child("language").data();
            if(language == "en-CA") language = "en-US"; // cause Apple does not respect Canada
        }
        if (info->second.find("urgency") != info->second.not_found())
            urgency = info->second.get_child("urgency").data();
        if (info->second.find("severity") != info->second.not_found())
            severity = info->second.get_child("severity").data();
        if (info->second.find("headline") != info->second.not_found())
            headline = info->second.get_child("headline").data();
        if (info->second.find("description") != info->second.not_found())
            description = info->second.get_child("description").data();
        if (info->second.find("instruction") != info->second.not_found())
            instruction = info->second.get_child("instruction").data();

        // gather all <area> nodes
        std::vector<boost::property_tree::ptree::iterator> areaNodes;
        for (auto it = info->second.begin(); it != info->second.end(); it++)
        {
            if (it->first == "area")
                areaNodes.push_back(it);
        }

        if (areaNodes.size() == 0) // No <area> nodes. Assume area is relevant
            areaCovered = 1;
        else
        {
            for (auto area : areaNodes)
            {
                if (area->second.find("polygon") != area->second.not_found())
                {
                    // check if polygon encomasses radio station
                    if (checkArea(std::move(area->second.get_child("polygon").data())))
                    {
                        areaCovered = 1;
                        break;
                    }
                }
            }
        }
        if (!areaCovered) // <area> not relevant
        {
            std::cout << "AREA NOT COVERED\n";
            return;
        }

        // std::string audioFileName = sent.data() + "-" + identifier.data() + "-" + language;
        std::string audioFileName = sent.data() + "-" + language;
        std::replace(audioFileName.begin(), audioFileName.end(), ':','-');
        // gather all <resource> nodes
        std::vector<boost::property_tree::ptree::iterator> resourceNodes;
        for (auto it = info->second.begin(); it != info->second.end(); ++it)
        {
            if (
                it->first == "resource" && 
                it->second.get_child("resourceDesc").data() == "Broadcast Audio" && 
                it->second.get_child("mimeType").data().substr(0, 5) == "audio")
            {
                resourceNodes.push_back(it);
            }
        }
        if (resourceNodes.size() == 0) // No <Resource> nodes. Perform TTS
        {
            std::cout << "NO RESOURCES \n";
            // if (severity != "Minor" || urgency != "Past")
            // {
                audioFileName += ".wav";
                std::string ttsText(std::move(headline) + ". " + std::move(description) + std::move(instruction));
                // std::cout << ttsText << std::endl;
                enqueueTTS(std::move(ttsText), audioFileName, language);
            // }
        }
        else
        {
            std::cout << "FOUND RESOURCES\n";
            for (auto resource : resourceNodes)
            {
                if (resource->second.find("resourceDesc") != resource->second.not_found())
                    resourceDesc = resource->second.get_child("resourceDesc").data(); // must be Broadcast Audio
                if (resource->second.find("mimeType") != resource->second.not_found())
                    mimeType = resource->second.get_child("mimeType").data(); // must be audio/mpeg, audio/wav, audio/x-wav, audio/x-ms-wma
                if (resource->second.find("size") != resource->second.not_found())
                    size = resource->second.get_child("size").data();
                if (resource->second.find("uri") != resource->second.not_found())
                {
                    uri = resource->second.get_child("uri").data();
                    boost::algorithm::trim(uri); // remove leading and trailing whitespaces
                }

                if (mimeType.substr(0, 5) != "audio") // <mimeType> is not audio
                    continue;
                if (resourceDesc != "Broadcast Audio") // <resourceDesc> is not Broadcast Audio
                    continue;

                std::string ext;
                if (mimeType == "audio/mpeg")
                    ext = ".mp3";
                else if (mimeType == "audio/wav")
                    ext = ".wav";
                else if (mimeType == "audio/x-wav")
                    ext = ".wav";
                else if (mimeType == "audio/x-ms-wma")
                    ext = ".wav";
                else
                    ext = ".wav";

                audioFileName += ext;
                if (resource->second.find("derefUri") != resource->second.not_found())
                {
                    std::cout << "BASE64 ENCODED AUDIO" << std::endl;
                    encodedData = resource->second.get_child("derefUri").data();
                    decodeToFile(std::move(encodedData), audioFileName);
                }

                if (uri.substr(0, 4) == "http")
                {
                    std::cout << "DOWNLOAD REMOTE AUDIO" << std::endl;
                    enqueueDwnld(uri, audioFileName);
                }
            }
        }
    }
}
