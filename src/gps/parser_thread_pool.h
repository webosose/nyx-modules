// Copyright (c) 2012 Jakob Progsch, Václav Zeman
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//   1. The origin of this software must not be misrepresented; you must not
//   claim that you wrote the original software. If you use this software
//   in a product, an acknowledgment in the product documentation would be
//   appreciated but is not required.
//
//   2. Altered source versions must be plainly marked as such, and must not be
//   misrepresented as being the original software.
//
//   3. This notice may not be removed or altered from any source
//   distribution.
//   4. Alterd source for worker thread joinable & nomenclature

#ifndef PARSER_THREAD_POOL_H
#define PARSER_THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#include <nyx/module/nyx_log.h>

class ParserThreadPool {

private:
    bool terminate;
    std::vector< std::thread > workers;
    std::condition_variable condition;
    std::mutex queue_mutex;
    std::queue< std::function<void()> > parser_tasks;
    unsigned int sleepFor;

public:
    ParserThreadPool(size_t threads, unsigned int sleepTime = 0);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ParserThreadPool();

};


inline ParserThreadPool::ParserThreadPool(size_t threads, unsigned int sleepTime)
    :   terminate(false)
    ,   sleepFor(sleepTime)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task = [] () {};

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->terminate || !this->parser_tasks.empty(); });
                        //if(this->terminate && this->parser_tasks.empty())
                        if (this->terminate)
                            return;
                        task = std::move(this->parser_tasks.front());
                        this->parser_tasks.pop();
                        if (sleepFor)
                            std::this_thread::sleep_for (std::chrono::seconds(sleepFor));
                    }

                    if (this->terminate == false)
                        task();
                }
            }
        );
}


template<class F, class... Args>
auto ParserThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if(terminate)
            throw std::runtime_error("enqueue on terminated ParserThreadPool");

        parser_tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

inline ParserThreadPool::~ParserThreadPool()
{
    {
        try {
            std::unique_lock<std::mutex> lock(queue_mutex);
            terminate = true;
            while(!this->parser_tasks.empty())
                this->parser_tasks.pop();
        }
        catch(const std::system_error& e) {
            nyx_error("MSGID_NMEA_PARSER", 0, "Exception occured:  %s", e.what());
        }
    }
    condition.notify_all();

    for (std::thread &worker: workers) {
        if (worker.joinable()) {
            try {
                worker.join();
            }
            catch (std::exception &e) {
               nyx_error("MSGID_NMEA_PARSER", 0, "Exception occured:  %s", e.what());
            }
        }
    }
}

#endif  //PARSER_THREAD_POOL_H
