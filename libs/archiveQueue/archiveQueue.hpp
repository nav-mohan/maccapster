#ifndef ARCHIVEQUEUE_HPP
#define ARCHIVEQUEUE_HPP

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <iostream>

struct ArchiveQueue
{
    std::queue<std::string> m_queue;
    std::mutex m_mut;
    std::thread m_workerThread;
    std::condition_variable m_cond;

    void Handler();
    void Push(const std::string& directoryPath);
    inline void Push(const char * directoryPath){return Push(std::string(directoryPath));}
    bool MoveFiles(std::vector<std::string>& files, std::string& directoryPath);

    ArchiveQueue() : m_workerThread(&ArchiveQueue::Handler,this){}
    ~ArchiveQueue();

    std::atomic_bool m_quit = ATOMIC_VAR_INIT(0);
    std::atomic_bool m_busy = ATOMIC_VAR_INIT(0);

    std::function<void(const std::string& directoryPath)> DoArchiving;
    std::function<void()> OnFinishArchiving;
};


#endif // ARCHIVEQUEUE_HPP
