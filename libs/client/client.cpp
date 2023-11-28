#include "client.hpp"

void Client::DoResolve()
{
    // start resolving
    basic_log("RESOLVING " + host_ + ":"+port_);
    endpoints_.clear();
    resolver_.async_resolve
    (
        host_,port_,
        [this](const boost::system::error_code& ec, boost::asio::ip::tcp::resolver::iterator endpoint_iter)
        {
            if(ec)
            {
                basic_log("ERROR RESOLVING " + host_ + ":"+port_);
                // DoReconnect();
                DoWait(5);
                DoResolve();
            }
            else
            {
                for (; endpoint_iter != boost::asio::ip::tcp::resolver::iterator(); ++endpoint_iter) {
                    endpoints_.push_back(*endpoint_iter);
                }
                basic_log("Client::DoResolve ACQUIRED " + std::to_string(endpoints_.size()) + "ENDPOINTS");
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
    std::stringstream ss;
    ss << "Client::DoConnect::CONNECTING TO " << ep;
    basic_log(ss.str());
    tcpSocket_.async_connect
    (
        ep,
        [this](const boost::system::error_code& ec)
        {
            if(!ec)
            {
                basic_log("Client::DoConnect SUCCESS. START TIMER " + host_ + ":" + port_ );
                // pmxTimer_.async_wait(OnTimerTimeout);
                // maybe move this to the constructor. do it just once and then at the end of every timeout
                pmxTimer_.async_wait([this](const boost::system::error_code ec){OnTimeout(ec);}); // this seems to trigger an infinite loop
                return DoRead();
            }
            else
            {
                basic_log("Client::DoConnect FAILED " + ec.what(), ERROR);
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
    basic_log("Client::DoRead WAITING..." );
    tcpSocket_.async_receive
    (
        boost::asio::buffer(tempBuffer_),
        [&](boost::system::error_code ec, size_t size)
        {
            if(ec)
            {
                basic_log("Client::DoRead ASYNC_RECEIVE ERROR ..." + ec.what(),ERROR);
                DoDisconnect();
                return DoConnect();
            }
            else
            {
                // basic_log("Client::DoRead RESET TIMER",DEBUG);
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
    // basic_log("Client::OnTimeout TIMER TIMEDOUT" + ec.what(),DEBUG);
    // const uint32_t timeLeft = pmxTimer_.expires_from_now().count()/1000000000;
    // std::cout << "TIME LEFT " << timeLeft << std::endl;
    // if(timeLeft > 0) return;
    if(!tcpSocket_.is_open())  // not sure if this is useful
    {
        // basic_log("Client::OnTimeout TIMER TIMED OUT WITH SOCKET CLOSED" + ec.what(),DEBUG);
        // DoDisconnect();
        // return DoConnect(); 
    }
    // this is a natural abort - we only need to extend the timer, no need to redefine onTimeOut callback
    if(!ec) // timer timed out naturally -- meaning the server is sending out XMLs
    {
        basic_log("Client::OnTimeout SERVER NOT SENDING!" + host_,WARN);
        DoDisconnect();
        // DoWait(5);
        // return DoConnect();
        pmxTimer_.expires_from_now(boost::posix_time::seconds(heartbeatFreq_));
    }
    // this is a premature abort - we have to redeclare the onTimeOut callback
    if(ec == boost::asio::error::operation_aborted) // not sure when all this happens - but it happens when client resets them timer
    {
        // basic_log("Client::OnTimeout operation_aborted " + ec.what(),DEBUG);
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
