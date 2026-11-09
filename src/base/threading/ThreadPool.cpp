#include "oor/base/threading/ThreadPool.hpp"

#include <algorithm>
#include <utility>

namespace oor::base
{

ThreadPool::ThreadPool(std::size_t workerCount)
{
    m_workers.reserve(workerCount);
    for (std::size_t index = 0; index < workerCount; ++index)
        m_workers.emplace_back([this](const std::stop_token& stopToken) { workerLoop(stopToken); });
}

ThreadPool::~ThreadPool()
{
    // std::jthread requests a stop and joins in its destructor; wake the workers so that they see the request.
    for (std::jthread& worker : m_workers)
        worker.request_stop();
    m_wake.notify_all();
}

void ThreadPool::submit(std::function<void()> task)
{
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push(std::move(task));
    }
    m_wake.notify_one();
}

std::size_t ThreadPool::defaultWorkerCount() noexcept
{
    return std::max<std::size_t>(1, std::thread::hardware_concurrency());
}

void ThreadPool::workerLoop(const std::stop_token& stopToken)
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_wake.wait(lock, stopToken, [this] { return !m_tasks.empty(); });
            if (m_tasks.empty())
                return; // stop requested and nothing left to do
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        task();
    }
}

} // namespace oor::base
