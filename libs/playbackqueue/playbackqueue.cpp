#include "playbackqueue.hpp"

void PBQueue::Handler()
{
    std::unique_lock<std::mutex> lock(mut_);

    do
    {
        cond_.wait(lock,[this](){
            return (busy_.load()==0 && (queue_.size()||quit_.load()));
        });
        if(queue_.size() && quit_.load()==0 && busy_.load()==0)
        {
            std::string filename = std::move(queue_.front());
            queue_.pop();
            busy_.store(1);
            lock.unlock();
            Play(filename); // blocking call
            if(!queue_.size()) PlayLast();
            lock.lock();
            busy_.store(0);
        }
    }
    while(quit_.load()==0);
}

void PBQueue::Push(const std::string& f)
{
    MsLogger<INFO>::get_instance().log_to_stdout("PBQueue::Push() " + f);
    MsLogger<INFO>::get_instance().log_to_file("PBQueue::Push() " + f);
    std::lock_guard<std::mutex> lock(mut_);
    if(!queue_.size() && !busy_.load()) PlayFirst();
    queue_.push(f);
    cond_.notify_one();
}

PBQueue::~PBQueue()
{
    MsLogger<INFO>::get_instance().log_to_stdout("PBQueue::~PBQueue()");
    MsLogger<INFO>::get_instance().log_to_file("PBQueue::~PBQueue()");
    std::unique_lock<std::mutex> lock(mut_);
    quit_.store(1);
    cond_.notify_one();
    lock.unlock();
    if(workerThread_.joinable()) workerThread_.join();
}