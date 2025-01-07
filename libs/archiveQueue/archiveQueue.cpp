#include "archiveQueue.hpp"
#include <exception>
#include <filesystem>

void ArchiveQueue::Handler()
{
    std::unique_lock<std::mutex> lock(m_mut);
    do
    {
        m_cond.wait(lock, [this](){
            return (m_busy.load() == 0 && (m_queue.size() || m_quit.load()));
        });
        if(m_queue.size() > 0 && m_quit.load() == 0 && m_busy.load() == 0)
        {
            std::string directoryPath = std::move(m_queue.front());
            m_queue.pop();
            m_busy.store(1);
            lock.unlock();
            DoArchiving(directoryPath);
            if(m_queue.size() == 0) OnFinishArchiving();
            lock.lock();
            m_busy.store(0);
        }
    } while (m_quit.load() == 0);
}

void ArchiveQueue::Push(const std::string& filename)
{
    printf("PUSHING DIRECTORY %s\n",filename.c_str());
    std::lock_guard<std::mutex> lock(m_mut);
    m_queue.push(filename);
    m_cond.notify_one();
}

ArchiveQueue::~ArchiveQueue()
{
    std::unique_lock<std::mutex> lock(m_mut);
    m_quit.store(1);
    m_cond.notify_one();
    lock.unlock();
    if(m_workerThread.joinable()) m_workerThread.join();
    printf("CLOSING ARCHIVEQUEUE\n");
}

// this could be a separate utility function 
// create a directory and move the vector<files> into this directory
bool ArchiveQueue::MoveFiles(std::vector<std::string>& files, std::string& directoryPath)
{
    // Create the directory (and append a suffix _i if it exists)
    int suffix = 0;
    std::string dp(directoryPath);
    while(std::filesystem::exists(std::filesystem::path(dp)))
        dp = directoryPath + "_" + std::to_string(++suffix);
    directoryPath = dp;

    // create the files into this directory
    try
    {
        std::filesystem::create_directory(directoryPath);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
    
    // move the files into this new directory
    bool atleastOneFileMoved = false;
    for(const std::string& file : files)
    {
        const std::string& fileName = std::filesystem::path(file).filename();
        try
        {
            std::filesystem::rename(file,directoryPath+"/"+fileName);
            atleastOneFileMoved = true;
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
    if(atleastOneFileMoved == false)
        std::filesystem::remove_all(directoryPath);

    return atleastOneFileMoved;
}
