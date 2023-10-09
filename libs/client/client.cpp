#include "client.hpp"

void Client::DoResolve()
{
    // start resolving
    std::cout << "RESOLVING " << host_ << ":" << port_ << std::endl;
    endpoints_.clear();
    resolver_.async_resolve
    (
        host_,port_,
        [this](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iter)
        {
            if(ec)
            {
                std::cout << "DoResolve::ERROR " << ec.what() << std::endl;
                // DoReconnect();
                DoWait(5);
                DoResolve();
            }
            else
            {
                for (; endpoint_iter != boost::asio::ip::tcp::resolver::iterator(); ++endpoint_iter) {
                    endpoints_.push_back(*endpoint_iter);
                }
                std::cout << "DoResolve::ACQUIRED " << endpoints_.size() << " ENDPOINTS" << std::endl;
                curEndpoint_ = endpoints_.begin();
                return DoConnect();
            }
        }
    );
}

void Client::DoConnect()
{
    // start connecting
    auto ep = *curEndpoint_;
    std::cout << "DoConnect::CONNECTING TO " << ep << std::endl;
    tcpSocket_.async_connect
    (
        ep,
        [this](const boost::system::error_code& ec)
        {
            if(!ec)
            {
                std::cout << "DoConnect::SUCCESS. START TIMER " << host_ << ":" << port_ << std::endl;
                // pmxTimer_.async_wait(OnTimerTimeout);
                // maybe move this to the constructor. do it just once and then at the end of every timeout
                pmxTimer_.async_wait([this](const boost::system::error_code ec){OnTimeout(ec);}); // this seems to trigger an infinite loop
                return DoRead();
            }
            else
            {
                std::cout << "DoConnect::FAILED " << ec.what() << std::endl;
                DoDisconnect();
                DoWait(5);
                return DoConnect();
            }
        }
    );
}

void Client::DoRead()
{
    // start reading
    std::cout << "DoRead::WAITING..." << std::endl;
    tcpSocket_.async_receive
    (
        boost::asio::buffer(tempBuffer_),
        [&](boost::system::error_code ec, size_t size)
        {
            if(ec)
            {
                std::cout << "ASYNC_RECEIVE ERROR " << ec.what().c_str() << std::endl;
                DoDisconnect();
                return DoConnect();
            }
            else
            {
                std::cout << "DoRead::RESET TIMER" << std::endl;
                // pmxTimer_.expires_after(std::chrono::seconds(heartbeatFreq_));
                pmxTimer_.expires_from_now(boost::posix_time::seconds(heartbeatFreq_));
                // std::cout.write(tempBuffer_.data(),size);
                // auto end = tempBuffer_.begin() + size; // std::next 
                appendXml(host_,std::move(tempBuffer_),size);
                return DoRead();
            }
        }
    );
}


void Client::OnTimeout(const boost::system::error_code ec)
{
    std::cout << "TIMER TIMEDOUT " << ec.what() << std::endl;
    // const uint32_t timeLeft = pmxTimer_.expires_from_now().count()/1000000000;
    // std::cout << "TIME LEFT " << timeLeft << std::endl;
    // if(timeLeft > 0) return;
    if(!tcpSocket_.is_open())  // not sure if this is useful
    {
        std::cout << "TIMER TIMED OUT WITH SOCKET CLOSED" << std::endl;
        // DoDisconnect();
        // return DoConnect(); 
    }
    // this is a natural abort - we only need to extend the timer, no need to redefine onTimeOut callback
    if(!ec) // timer timed out naturally -- meaning the server is sending out XMLs
    {
        std::cout << "OnTimerTimeout::SERVER NOT SENDING!" << host_ << std::endl;
        DoDisconnect();
        // DoWait(5);
        // return DoConnect();
        pmxTimer_.expires_from_now(boost::posix_time::seconds(heartbeatFreq_));
    }
    // this is a premature abort - we have to redeclare the onTimeOut callback
    if(ec == boost::asio::error::operation_aborted) // not sure when all this happens - but it happens when client resets them timer
    {
        std::cout << "OnTimerTimeout::operation_aborted" << std::endl;
        pmxTimer_.expires_from_now(boost::posix_time::seconds(heartbeatFreq_));
        pmxTimer_.async_wait([this](const boost::system::error_code ec){OnTimeout(ec);}); // this seems to trigger an infinite loop
    }
    return;
};


void Client::DoReconnect()
{
    DoDisconnect();
    DoWait(5);
    return DoConnect();
}

void Client::DoDisconnect()
{
    tcpSocket_.~basic_stream_socket();
    tcpSocket_ = boost::asio::ip::tcp::socket(ioContext_);
}

void Client::DoWait(uint32_t sec)
{
    boost::asio::steady_timer timer(ioContext_, std::chrono::seconds(sec));
    timer.wait();
}
