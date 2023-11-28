#if !defined(CLIENT_HPP)
#define CLIENT_HPP

// XmlHandler::regexpattern_ cannot handle a string of two alerts eg:
        // <xml>
        // <alert>...</alert>
        // <xml>
        // <alert>...</alert>
// keep this < 1500. 
#define BUFFERSIZE 1600


#include <boost/asio.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <functional>
#include "mslogger.hpp"
class Client
{
public:
    Client(boost::asio::io_context& ioc) 
    :   ioContext_(ioc)
    ,   tcpSocket_(ioc)
    ,   resolver_(ioContext_)
    ,   tempBuffer_(BUFFERSIZE,0)
    ,   pmxTimer_(ioc)
    {}
    ~Client(){
        basic_log("Client::~Client " + host_);
    }

    void inline setTimerFrequency(uint32_t heartbeat_freq){
        heartbeatFreq_ = heartbeat_freq;
        // pmxTimer_.expires_after(std::chrono::seconds(heartbeatFreq_));
        pmxTimer_.expires_from_now(boost::posix_time::seconds(heartbeatFreq_));

    }

    void inline start(){DoResolve();}
    void DoDisconnect();

    void inline set_endpoint(std::string host, std::string port){
        host_ = host;
        port_ = port;
    }

    std::function<void(std::string& host, const std::vector<char>&& tempBuffer, size_t size)> appendXml;

private:
    std::string host_;
    std::string port_;
    uint32_t heartbeatFreq_;
    boost::asio::io_context& ioContext_;
    boost::asio::ip::tcp::resolver resolver_;
    std::vector<boost::asio::ip::tcp::endpoint> endpoints_;
    std::vector<boost::asio::ip::tcp::endpoint>::iterator curEndpoint_;
    boost::asio::ip::tcp::socket tcpSocket_;
    std::vector<char> tempBuffer_; // this will be touched by the TCP socket
    // boost::asio::high_resolution_timer pmxTimer_;
    boost::asio::deadline_timer pmxTimer_;
    void DoResolve();
    void DoConnect();
    void DoRead();
    void DoReconnect();
    void DoWait(uint32_t sec = 5);
    void OnTimeout(const boost::system::error_code ec);
};
#endif // CLIENT_HPP
