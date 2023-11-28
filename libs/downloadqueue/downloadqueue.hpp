#if !defined(DOWNLOADQUEUE_HPP)
#define DOWNLOADQUEUE_HPP

#include <iostream>
#include <string>
#include <cstdlib>

#include <boost/beast.hpp>
#include <boost/asio.hpp>

#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include "load_certificates.hpp"

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <functional>

#include <regex>

#include "mslogger.hpp"

struct Downloadable
{
    std::string host_;
    std::string target_;
    std::string port_;
    std::string filename_;
    Downloadable(){}
    Downloadable(const std::string& h, const std::string& t, const std::string& p, const std::string& f)    : host_(h), target_(t), port_(p), filename_(f) {}
    Downloadable(std::string&& h, std::string&& t, std::string&& p, std::string&& f) noexcept               : host_(h), target_(t), port_(p), filename_(f) {}
};

struct DWQueue
{
    std::queue<Downloadable> queue_;
    std::mutex mut_;
    std::thread workerThread_;
    std::condition_variable cond_;
    boost::asio::io_context& ioc_;
    void Handler();
    void Push(const Downloadable& d);
    void Push(Downloadable&& d);
    void Download(const Downloadable& d);
    void SecureDownload(const Downloadable d);
    DWQueue(boost::asio::io_context& ioc, std::function<void(std::string filename)>&& lambda, std::string&& certbuffer);
    ~DWQueue();
    std::atomic_bool busy_ = ATOMIC_VAR_INIT(0);
    std::atomic_bool quit_ = ATOMIC_VAR_INIT(0);
    std::function<void(std::string filename)>OnDownload;

    boost::asio::ssl::context ctx_;

};

#endif // DOWNLOADQUEUE_HPP