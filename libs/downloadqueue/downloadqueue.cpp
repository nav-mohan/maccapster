#include "downloadqueue.hpp"

DWQueue::DWQueue(boost::asio::io_context& ioc, std::function<void(std::string filename)>&& lambda, std::string&& certbuffer) 
: ioc_(ioc), workerThread_(&DWQueue::Handler, this), OnDownload(std::move(lambda)), ctx_(boost::asio::ssl::context::tlsv12_client)
{
    load_root_certificates(ctx_,std::move(certbuffer));
    ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
}

DWQueue::~DWQueue()
{
    printf("DWQueue::~DWQueue\n");
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
            lock.unlock();
            if (d.port_ == "443") SecureDownload(d);
            else Download(d);
            lock.lock();
            busy_.store(0);
        }
//        else 
//            printf("waiting...\n");
    }
    while(quit_.load()==0);
    printf("EXIT WHILE\n");
}


void DWQueue::Push(const Downloadable& d)
{
    printf("PUSH1 %s\n",d.filename_.c_str());
    std::lock_guard<std::mutex> lock(mut_);
    queue_.push(d);
    cond_.notify_one();
}


void DWQueue::Push(Downloadable&& d)
{
    printf("PUSH2 %s\n",d.filename_.c_str());
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
        printf("Shutdown Failed: %s\n",d.filename_.c_str());
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
        printf("Failed to set SNI Hostname: %s\n",ec.what().c_str());
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
        printf("SSL Shutdown Failed: %s\n",ec.what().c_str());
        return;
    }

    FILE *outfile = fopen(d.filename_.c_str(),"wb");
    if(!outfile) return;
    int bytes = fwrite(res.body().c_str(),1,res.body().size(),outfile);
    fclose(outfile);
    if(!bytes) return;
    OnDownload(d.filename_);
}
