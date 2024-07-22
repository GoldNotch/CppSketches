/*---------------------------
Async Logger
Given interface ILogger with method Log
and there is a sync implementation of that.
But it's measured that sync implementation is slow,
calling thread is blocked while logging.
So, you should implement async implementation with low latency Log method
-----------------------------*/

// sleep time for sync logging in milliseconds
#define SYNC_LOG_SLEEP_TIME 100

#include <string>
#include <queue>
#include <iostream>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <array>
#include <functional>
#include <optional>

#include <catch2/catch_test_macros.hpp>

/// @brief interface for loggers
struct ILogger
{
    virtual ~ILogger() = default;
    virtual void Log(const std::string_view& msg) = 0;
};

std::atomic_int log_call_counter = 0;

/// @brief synchronous logger with slow Log implementation
struct SyncLogger : public ILogger
{
    void Log(const std::string_view & msg) override
    {
        #ifdef CONSOLE_OUT
        std::cout << msg << std::endl;
        #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(SYNC_LOG_SLEEP_TIME));
        log_call_counter++;
    }
};


template<typename LoggerT, size_t threads_num>
size_t TestLogger()
{
    log_call_counter = 0;
    const size_t messages_count = 100;
    {
        auto logger = std::make_shared<LoggerT>();
        auto LoggingFunc = [logger, messages_count](int idx)
            {
                for (int i = 0; i < messages_count; ++i)
                    logger->Log(std::to_string(idx) + ": Record " + std::to_string(i));
            };

        std::array<std::thread, threads_num> pool;
        for(int i = 0; i < threads_num; ++i)
            pool[i] = std::thread(LoggingFunc, i);

        for(auto && th : pool)
            th.join();
    }
    REQUIRE(log_call_counter == threads_num * messages_count);
    return log_call_counter;// == threads_num * messages_count;
}

//------------------------ Solution ----------------------


/// @brief Asynchronous optimization for SyncLogger which doesn't block calling thread
struct CondVarLogger : public ILogger
{
    CondVarLogger()
        : th(std::bind(&CondVarLogger::ThreadMain, this))
    {}

    ~CondVarLogger() override
    {
        //std::cout << "Destroy\n";
        {
            std::lock_guard<std::mutex> lk{ m };
            is_running = false;
        }
        cv.notify_one();
        //std::cout << "Join\n";
        th.join();
        //std::cout << "Destroyed\n";
    }

    void Log(const std::string_view& msg) override
    {
        if (is_running)
        {
            std::lock_guard<std::mutex> lk{ m };
            q.push(std::string(msg));
        }
        cv.notify_one();
    }

private:
    std::atomic_bool is_running = true; ///< flag to check if objects if destroying
    std::queue<std::string> q; ///< messages queue
    std::mutex m; ///< mutex to protect queue
    std::condition_variable cv;
    std::thread th; ///< thread for logging

private:
    void ThreadMain()
    {
        SyncLogger logger;
        while (is_running)
        {
            {
                std::unique_lock<std::mutex> lk{m};
                cv.wait(lk, [this]{return !is_running || !q.empty();});
                if (!is_running)
                 break;
                if (!q.empty()){
                    logger.Log(q.front());
                    q.pop();
                }
            }
        }

        std::lock_guard lk{m};
        while (!q.empty())
        {
            logger.Log(q.front());
            q.pop();
        }
    }
};



TEST_CASE( "Test CondVarLogger1", "[CondVarLogger]" ) 
{
    /*TestLogger<CondVarLogger, 1>();
    TestLogger<CondVarLogger, 2>();
    TestLogger<CondVarLogger, 4>();
    TestLogger<CondVarLogger, 8>();*/
}