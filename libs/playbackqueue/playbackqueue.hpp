#include <iostream>
#include <string>
#include <unistd.h>

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>

#include "mslogger.hpp"

struct PBQueue
{
    std::queue<std::string> queue_;
    std::mutex mut_;
    std::thread workerThread_;
    std::condition_variable cond_;
    void Handler();
    void Push(const std::string& filename);
    void Push(const char *filename){Push(std::string(filename));};
    // void Play(const std::string& filename);
    PBQueue() : workerThread_(&PBQueue::Handler, this){};
    ~PBQueue();
    std::atomic_bool quit_ = ATOMIC_VAR_INIT(0);
    std::atomic_bool busy_ = ATOMIC_VAR_INIT(0);

    std::function<void(const std::string& filename)> Play;
    std::function<void()> PlayFirst;
    std::function<void()> PlayLast;

};

