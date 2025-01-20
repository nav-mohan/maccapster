#include "dailyWorker.hpp"

DailyWorker::~DailyWorker()
{
    std::unique_lock<std::mutex> lock(mut_);
    quit_.store(1);
    cond_.notify_one();
    lock.unlock();
    if(workerThread_.joinable()) workerThread_.join();
}

void DailyWorker::Handler()
{
    while(!quit_.load())
    {
        std::unique_lock<std::mutex> lock(mut_);
        std::chrono::system_clock::time_point nextExecutionTime = GetNextExecutionTimePoint();
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        cond_.wait_until(
            lock, nextExecutionTime, [&](){
                return std::chrono::system_clock::now() >= nextExecutionTime;
            }
        );
        DoTask();
    } 
}

// get the std::chrono::time_point for the upcoming 12:01 AM
std::chrono::system_clock::time_point DailyWorker::GetNextExecutionTimePoint() const 
{
    std::chrono::time_point now         = std::chrono::system_clock::now(); 
    std::time_t             currTime    = std::chrono::system_clock::to_time_t(now);
    std::tm                 localTime   = *std::localtime(&currTime);
    // localTime is equal to now

    localTime.tm_hour   = scheduleTime_.hour;
    localTime.tm_min    = scheduleTime_.min;
    localTime.tm_sec    = scheduleTime_.sec;
    // localTime is equal to today-midnight. 

    std::chrono::time_point nextExecutionTimePoint = std::chrono::system_clock::from_time_t(std::mktime(&localTime));
    if(nextExecutionTimePoint <= now) 
        nextExecutionTimePoint += std::chrono::hours(24);
    std::cout << "Current time : " << std::chrono::system_clock::to_time_t(now) << std::endl;
    std::cout << "Next Task at : " << std::chrono::system_clock::to_time_t(nextExecutionTimePoint) << std::endl;

    return nextExecutionTimePoint;
}

void DailyWorker::SetScheduleTime(uint hour, uint min, uint sec)
{
    std::lock_guard<std::mutex> lock(mut_);
    scheduleTime_ = {hour,min,sec};
    cond_.notify_one();
    basic_log("SET SCHEDULE-TIME TO " + std::to_string(hour) + ":" + std::to_string(min) + ":" + std::to_string(sec));
}
