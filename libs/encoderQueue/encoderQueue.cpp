#include "encoderQueue.hpp"

void EncQueue::Handler()
{
    std::unique_lock<std::mutex> lock(mut_);

    do
    {
        cond_.wait(lock, [this](){
            return (busy_.load() == 0 && (queue_.size()||quit_.load()));
        });
        if(queue_.size() && quit_.load()==0 && busy_.load()==0)
        {
            std::string filename = std::move(queue_.front());
            queue_.pop();
            busy_.store(1);
            history_.push_back(filename);
            lock.unlock();
            Encode(filename); // this blocks the workerThread_
            lock.lock();
            busy_.store(0);
            if(queue_.size()==0) 
            {
                OnFinish();
                history_.clear();
            }
        }
    } while (quit_.load() == 0);
}

void EncQueue::Push(const std::string& f)
{
    std::lock_guard<std::mutex> lock(mut_);
    queue_.push(f);
    cond_.notify_one();
}

EncQueue::~EncQueue()
{
    std::unique_lock<std::mutex> lock(mut_);
    quit_.store(1);
    cond_.notify_one();
    lock.unlock();
    if(workerThread_.joinable()) workerThread_.join();
}