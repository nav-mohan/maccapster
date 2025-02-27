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
            history_.push_back(filename); // read/write history when lock is held
            lock.unlock();
            Play(filename); // blocking call
            lock.lock();
            busy_.store(0);
            if(!queue_.size()) 
            {
                OnFinish();
                history_.clear(); // read/write history when lock is held
            }
        }
    }
    while(quit_.load()==0);
}

void PBQueue::Push(const std::string& f)
{
    basic_log("PBQueue::Push() " + f);
    std::lock_guard<std::mutex> lock(mut_);
    if(!queue_.size() && !busy_.load()) PlayFirst();
    queue_.push(f);
    cond_.notify_one();
}

PBQueue::~PBQueue()
{
    basic_log("PBQueue::~PBQueue()");
    std::unique_lock<std::mutex> lock(mut_);
    quit_.store(1);
    cond_.notify_one();
    lock.unlock();
    if(workerThread_.joinable()) workerThread_.join();
}