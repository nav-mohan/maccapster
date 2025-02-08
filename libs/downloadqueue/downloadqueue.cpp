#include "downloadqueue.hpp"

DWQueue::DWQueue(boost::asio::io_context& ioc, std::function<void(std::string filename)>&& lambda, std::string&& certbuffer) 
: ioc_(ioc), workerThread_(&DWQueue::Handler, this), OnDownload(std::move(lambda)), ctx_(boost::asio::ssl::context::tlsv12_client)
{
    load_root_certificates(ctx_,std::move(certbuffer));
    ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
}

DWQueue::~DWQueue()
{
    basic_log("DWQueue::~DQQueue()");
    std::unique_lock<std::mutex> lock(mut_);
    quit_.store(1);
    cond_.notify_one();
    lock.unlock();
    workerThread_.join();
}


void DWQueue::Handler()
{
    std::unique_lock<std::mutex> lock(mut_);
    do
    {
        cond_.wait(lock,[this](){
            return (busy_.load()==0 && (queue_.size()||quit_.load()));
        });
        if(queue_.size() > 0 && busy_.load()==0 && quit_.load()==0)
        {
            Downloadable d = std::move(queue_.front());
            queue_.pop();
            busy_.store(1);
            history_.push_back(d); // history is modified when lock is held
            lock.unlock();
            if (d.port_ == "443") SecureDownload(d);
            else Download(d);
            lock.lock();
            busy_.store(0);
            if(queue_.size() == 0) 
            {
                history_.clear(); // history is modified when lock is held
            }
        }
//        else 
//            printf("waiting...\n");
    }
    while(quit_.load()==0);
    basic_log("DWQueue::Handler() exit while loop",WARN);
}


void DWQueue::Push(const Downloadable& d)
{
    basic_log("DWQueue::Push() " + d.filename_,INFO);
    std::lock_guard<std::mutex> lock(mut_);
    queue_.push(d);
    cond_.notify_one();
}


void DWQueue::Push(Downloadable&& d)
{
    basic_log("DWQueue::Push() " + d.filename_,INFO);
    std::lock_guard<std::mutex> lock(mut_);
    queue_.push(std::move(d));
    cond_.notify_one();
}


void DWQueue::Download(const Downloadable& d)
{
    boost::asio::ip::tcp::resolver resolver(ioc_);
    boost::beast::tcp_stream stream(ioc_);
    boost::beast::http::request<boost::beast::http::empty_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;

    req.version(11);
    req.method(boost::beast::http::verb::get);
    req.target(d.target_);
    req.set(boost::beast::http::field::host, d.host_);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    auto const results = resolver.resolve(d.host_,d.port_);
    stream.connect(results);

    boost::beast::http::write(stream,req);

    boost::beast::flat_buffer buffer;

    boost::beast::http::read(stream,buffer,res);

    boost::beast::error_code ec;
    stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both,ec);
    if(ec && ec != boost::beast::errc::not_connected)
    {
        basic_log("DWQueue::Download() Shutdown Failed " + d.filename_,WARN);
        return;
    }

    FILE *outfile = fopen(d.filename_.c_str(),"wb");
    if(!outfile) return;
    int bytes = fwrite(res.body().c_str(),1,res.body().size(),outfile);
    fclose(outfile);
    if(!bytes) return;
    OnDownload(d.filename_);
}


void DWQueue::SecureDownload(const Downloadable d)
{
    boost::asio::ip::tcp::resolver resolver(ioc_);
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc_,ctx_);

    if(!SSL_set_tlsext_host_name(stream.native_handle(), d.host_.c_str()))
    {
        boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
        basic_log("DWQueue::SecureDownload() Failed to get SNI Hostname: " + ec.what(),ERROR);
        return;
    }

    auto const results = resolver.resolve(d.host_,d.port_);
    boost::beast::get_lowest_layer(stream).connect(results);
    stream.handshake(boost::asio::ssl::stream_base::client);

    boost::beast::http::request<boost::beast::http::empty_body> req;
    boost::beast::http::response<boost::beast::http::string_body> res;

    req.version(11);
    req.method(boost::beast::http::verb::get);
    req.target(d.target_);
    req.set(boost::beast::http::field::host, d.host_);
    req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    boost::beast::http::write(stream,req);
    boost::beast::flat_buffer buffer;
    boost::beast::http::read(stream, buffer, res);

    boost::beast::error_code ec;
    stream.shutdown(ec);
    if(ec && ec != boost::asio::error::eof)
    {
        basic_log("DWQueue::SecureDownload() SSL Shutdown failed: " + ec.what(),ERROR);
        return;
    }

    FILE *outfile = fopen(d.filename_.c_str(),"wb");
    if(!outfile) return;
    int bytes = fwrite(res.body().c_str(),1,res.body().size(),outfile);
    fclose(outfile);
    if(!bytes) return;
    OnDownload(d.filename_);
}
