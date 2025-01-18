#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>

// EncQueue has it's own dedicated thread
// EncQueue is triggered by TTSQueue
// we will create a separate ZipQueue which is triggered once a day
// EncQueue only handles WAV files
// whereas the ZipQueue will handle MP3 and XML files
// EncQueue will also handle deletion of oriignal WAV files based on return code of encoder
// ZipQueue will handle creation of new folder, moving 1-day-old files to new-folder, zipping, deleting new-folder, and moving zipped tarball into another folder
struct EncQueue
{
    std::queue<std::string> queue_; // a queue of WAV filepaths 
    std::mutex mut_;
    std::thread workerThread_;
    std::condition_variable cond_;
    void Handler();
    void Push(const std::string& filename);
    void Push(const char * filename){Push(std::string(filename));}
    EncQueue() : workerThread_(&EncQueue::Handler, this){};
    ~EncQueue();
    std::atomic_bool quit_ = ATOMIC_VAR_INIT(0);
    std::atomic_bool busy_ = ATOMIC_VAR_INIT(0);

    std::function<void(const std::string& filename)> Encode;
    std::function<void()> OnFinish; // upon emptying the queue, delete the original WAV file

    std::vector<const std::string> history_; // a record of all the files we've encoded so far. It is cleared after calling OnFinish()
    inline std::vector<const std::string> GetHistory() const {return history_;}
};
