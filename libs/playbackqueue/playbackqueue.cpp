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
    std::cout << "PBQueue::Push " << f << std::endl;
    std::lock_guard<std::mutex> lock(mut_);
    if(!queue_.size() && !busy_.load()) PlayFirst();
    queue_.push(f);
    cond_.notify_one();
}

PBQueue::~PBQueue()
{
    std::unique_lock<std::mutex> lock(mut_);
    std::cout << "PBQueue::~PBQueue" << std::endl;
    quit_.store(1);
    cond_.notify_one();
    lock.unlock();
    std::cout << "JOINIGN " << quit_.load() << std::endl;
    if(workerThread_.joinable()) workerThread_.join();
    std::cout << "PBQueue::~PBQueue" << std::endl;
}