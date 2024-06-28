#include <string>
#include <queue>
#include <iostream>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <array>

/// @brief interface for loggers
struct ILogger
{
    virtual ~ILogger() = default;
    virtual void Log(const std::string& msg) = 0;
};

/// @brief synchronous logger with slow Log implementation
struct SyncLogger : public ILogger
{
    void Log(const std::string & msg) override
    {
        std::cout << msg << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

};

/// @brief Asynchronous optimization for SyncLogger which doesn't block calling thread
struct AsyncLogger : public ILogger
{
    AsyncLogger()
        : logger(new SyncLogger)
        , is_running(true)
        , th([this] {ThreadMain(); })
    {}

    ~AsyncLogger()
    {
        std::cout << "Destroy\n";
        {
            std::lock_guard<std::mutex> lk{ m };
            is_running = false;
            cv.notify_all();
        }
        std::cout << "Join\n";
        th.join();

        std::cout << "Destroyed\n";
    }

    void Log(const std::string& msg) override
    {
        std::cout << msg << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (is_running)
        {
            std::lock_guard<std::mutex> lk{ m };
            q.push(msg);
            cv.notify_all();
        }
    }

private:
    std::unique_ptr<ILogger> logger;
    bool is_running = false;
    std::queue<std::string> q;

    std::mutex m;
    std::condition_variable cv;

    std::thread th;

private:

    void ThreadMain()
    {
        while (is_running)
        {
            {
                std::unique_lock<std::mutex> lk{m};
                cv.wait(lk, [this]{return !is_running || !q.empty();});
                if (!q.empty()){
                    logger->Log(q.front());
                    q.pop();
                }
            }
        }

        while (!q.empty())
        {
            logger->Log(q.front());
            q.pop();
        }
    }
};


int main()
{
    auto logger = std::make_shared<AsyncLogger>();
    auto threadMain = [logger](int idx)
        {
            for (int i = 0; i < 100; ++i)
                logger->Log(std::to_string(idx) + ": Record " + std::to_string(i));
        };

    std::array<std::thread, 8> pool;
    for(int i = 0; i < 8; ++i)
        pool[i] = std::thread(threadMain, i);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    threadMain(-1);

    for(auto && th : pool)
        th.join();

    return 0;
}