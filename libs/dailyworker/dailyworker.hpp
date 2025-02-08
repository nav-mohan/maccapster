#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <condition_variable>
#include <mutex>

#include "mslogger.hpp"

// DailyWorker executes DoTask() once a day at the scheduleTime_
// WaitFunctor is a functor that returns true/false that indicates if there are any external wait-signals on our DailyWorker
// it returns true only if all the other queues (such as EncoderQueue, DownloadQueue, PlaybackQueue) are not busy
// it's templated in because it gets called within a while loop

template<typename WaitFunctor>
struct DailyWorker
{
    std::mutex mut_;
    std::thread workerThread_;
    std::condition_variable cond_;
    void Handler();
    DailyWorker(WaitFunctor functor) : 
        workerThread_(&DailyWorker::Handler, this), waiter_(functor) {};
    ~DailyWorker();
    std::atomic_bool quit_ = ATOMIC_VAR_INIT(0);
    std::atomic_bool reset_ = ATOMIC_VAR_INIT(0);

    std::function<void()> DoTask;
    void SetScheduleTime(uint hour, uint min, uint sec);
    std::chrono::system_clock::time_point GetNextExecutionTimePoint() const;

    struct TimeOfDay{
        uint hour;
        uint min;
        uint sec;
    };
    TimeOfDay scheduleTime_ = {0,1,0}; // default time is 12:01 AM

    WaitFunctor waiter_;
};

template<typename WaitFunctor>
DailyWorker<WaitFunctor>::~DailyWorker()
{
    std::unique_lock<std::mutex> lock(mut_);
    quit_.store(1);
    cond_.notify_one();
    lock.unlock();
    if(workerThread_.joinable()) workerThread_.join();
}

template<typename WaitFunctor>
void DailyWorker<WaitFunctor>::Handler()
{
    std::chrono::system_clock::time_point nextExecutionTime = GetNextExecutionTimePoint();
    while(!quit_.load())
    {
        std::unique_lock<std::mutex> lock(mut_);
        reset_.store(0);

        cond_.wait_until(
            lock, nextExecutionTime, [&](){
                return (reset_.load() || std::chrono::system_clock::now() >= nextExecutionTime);
            }
        );
        if(!waiter_()) 
        {
            // if there is an external wait-signal, then postpone by 30 seconds
            nextExecutionTime = std::chrono::system_clock::now() + std::chrono::seconds(30);
            continue;
        }
        nextExecutionTime = GetNextExecutionTimePoint();
        if(reset_.load()) 
        {
            // if the scheduledTime has been changed, then go back and check if now() >= scheduledTime
            continue; 
        }
        DoTask();
    } 
}

// get the std::chrono::time_point for the upcoming 12:01 AM
template<typename WaitFunctor>
std::chrono::system_clock::time_point DailyWorker<WaitFunctor>::GetNextExecutionTimePoint() const 
{
    printf("START GetNextExecutionTimePoint\n");
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
    std::cout << "NEXT TASK IN " << 
        std::chrono::system_clock::to_time_t(nextExecutionTimePoint) 
        - std::chrono::system_clock::to_time_t(now) << " SECONDS" 
        << std::endl;

    return nextExecutionTimePoint;
}

template<typename WaitFunctor>
void DailyWorker<WaitFunctor>::SetScheduleTime(uint hour, uint min, uint sec)
{
    printf("START SetScheduleTime \n");
    scheduleTime_ = {hour,min,sec};
    reset_.store(1);
    cond_.notify_one();
    basic_log("SET SCHEDULE-TIME TO " + std::to_string(hour) + ":" + std::to_string(min) + ":" + std::to_string(sec));
}


/**
 * @brief 
 *not sure how to generalize this to accomodate both - specific timestamp 
 as well as speicifc time-interval. for macapster i dont need to be worried about 
 changing the timestamp. it'll be the same timestamp every time but, if i want to make a 
 calendar (for the scheduler-recorder app) then i should accomodate multiple timestamps
 or maybe in that case, each Task is an object in of itself, inserted into a priority_queue<Task>
 sorted by time and the top-most item's timestamp is checked every minute to see if it's time to 
 execeute. we must check periodically because if the user schedules a new Task earlier than the 
 front-most task, then our front-most timestamp has changed.
 */
