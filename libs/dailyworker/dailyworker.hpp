#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <condition_variable>
#include <mutex>

#include "mslogger.hpp"

// DailyWorker executes DoTask() once a day at the scheduleTime_

struct DailyWorker
{
    std::mutex mut_;
    std::thread workerThread_;
    std::condition_variable cond_;
    void Handler();
    DailyWorker() : workerThread_(&DailyWorker::Handler, this){};
    ~DailyWorker();
    std::atomic_bool quit_ = ATOMIC_VAR_INIT(0);

    std::function<void()> DoTask;
    void SetScheduleTime(uint hour, uint min, uint sec);
    std::chrono::system_clock::time_point GetNextExecutionTimePoint() const;

    struct TimeOfDay{
        uint hour;
        uint min;
        uint sec;
    };
    TimeOfDay scheduleTime_ = {0,1,0}; // default time is 12:01 AM
};

